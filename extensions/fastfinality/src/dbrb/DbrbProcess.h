/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/dbrb/Messages.h"
#include "MessageSender.h"
#include "TransactionSender.h"
#include "catapult/functions.h"
#include "catapult/net/PacketWriters.h"
#include "catapult/ionet/PacketHandlers.h"

namespace catapult { namespace thread { class IoThreadPool; }}

namespace catapult { namespace dbrb {

	/// Struct that encapsulates all necessary quorum counters and their update methods.
	struct QuorumManager {

		struct QuorumKey {
		public:
			dbrb::Sequence Sequence;

			QuorumKey(const dbrb::Sequence& sequence) {
				Sequence = sequence;
			};

			QuorumKey(const dbrb::View& view) {
				Sequence = dbrb::Sequence::fromViews({ view }).value();
			};

			QuorumKey(const std::pair<dbrb::View, dbrb::Sequence>& pair) {
				Sequence = dbrb::Sequence::fromViews({ pair.first }).value();
				bool appended = Sequence.tryAppend(pair.second);
				if (!appended)
					CATAPULT_LOG(warning) << "[DBRB] QUORUM: Quorum key was not formed correctly (failed to append " << pair.second << " to " << pair.first << ")";
			};

			bool operator<(const QuorumKey& other) const {
				const auto& thisData = Sequence.data();
				const auto& otherData = other.Sequence.data();

				return std::lexicographical_compare(
						thisData.begin(), thisData.end(),
						otherData.begin(), otherData.end(),
						[](const dbrb::View& a, const dbrb::View& b){
							return std::lexicographical_compare(
									a.Data.begin(), a.Data.end(),
									b.Data.begin(), b.Data.end());
						});
			}

			bool operator==(const QuorumKey& other) const {
				return Sequence == other.Sequence;
			}
			bool operator==(const dbrb::Sequence& sequence) const {
				return Sequence == sequence;
			}
			bool operator==(const dbrb::View& view) const {
				return Sequence == dbrb::Sequence::fromViews({ view }).value();
			}
			bool operator==(const std::pair<dbrb::View, dbrb::Sequence>& pair) const {
				auto sequence = dbrb::Sequence::fromViews({ pair.first }).value();
				sequence.tryAppend(pair.second);
				return Sequence == sequence;
			}

			friend std::ostream& operator<<(std::ostream& out, const QuorumKey& quorumKey) {
				out << quorumKey.Sequence;
				return out;
			}
		};

	public:
		/// Maps views to numbers of received ReconfigConfirm messages for those views.
		std::map<QuorumKey, std::set<ProcessId>> ReconfigConfirmCounters;

		/// Maps view-sequence pairs to numbers of received Proposed messages for those pairs.
		std::map<QuorumKey, std::set<ProcessId>> ProposedCounters;

		/// Maps view-sequence pairs to processes with their signatures that received Converged messages for those pairs.
		std::map<QuorumKey, std::map<ProcessId, Signature>> ConvergedSignatures;

		/// Maps views to received StateUpdate messages for those views.
		std::map<QuorumKey, std::set<StateUpdateMessage>> StateUpdateMessages;

		/// Maps views to sets of pairs of respective process IDs and payload hashes received from Acknowledged messages.
		std::map<QuorumKey, std::set<std::pair<ProcessId, Hash256>>> AcknowledgedPayloads;

		/// Maps views to sets of process IDs ready for delivery.
		std::map<QuorumKey, std::set<ProcessId>> DeliveredProcesses;

		/// Overloaded methods for updating respective counters.
		/// Returns whether the quorum has just been collected on this update.

		bool update(const ReconfigConfirmMessage& message) {
			return update(ReconfigConfirmCounters, message.View, message.Sender, message.View.quorumSize(), "RECONFIG CONFIRM");
		};

		bool update(const ProposeMessage& message) {
			const auto keyPair = std::make_pair(message.ReplacedView, message.ProposedSequence);
			return update(ProposedCounters, keyPair, message.Sender, message.ReplacedView.quorumSize(), "PROPOSE");
		};

		bool update(const ConvergedMessage& message) {
			auto sequence = Sequence::fromViews({message.ReplacedView}).value();
			sequence.tryAppend(message.ConvergedSequence);
			const auto quorumKey = QuorumKey(sequence);

			CATAPULT_LOG(debug) << "[DBRB] CONVERGED UPDATE: Comparing " << quorumKey << " to every existing key in map:";
			for (const auto& [key, _] : ConvergedSignatures)
				CATAPULT_LOG(debug) << "[DBRB] CONVERGED UPDATE: == " << key << " : " << (key == quorumKey);

			auto& map = ConvergedSignatures[quorumKey];

			CATAPULT_LOG(debug) << "[DBRB] CONVERGED UPDATE: Map at (" << quorumKey << ") is:";
			for (const auto& [sender, signature] : map)
				CATAPULT_LOG(debug) << "[DBRB] CONVERGED UPDATE: " << sender << " : " << signature;

			if (map.count(message.Sender))
				return false; // This process has already sent a Converged message; do nothing, quorum is NOT triggered on this message.

			map[message.Sender] = message.Signature;
			CATAPULT_LOG(debug) << "[DBRB] CONVERGED UPDATE: Inserted " << message.Sender << " : " << map.at(message.Sender);

			CATAPULT_LOG(warning) << "[DBRB] CONVERGED UPDATE: Current ConvergedSignatures map is:";
			for (const auto& [sequence, value] : ConvergedSignatures) {
				CATAPULT_LOG(debug) << "[DBRB] CONVERGED UPDATE: (" << sequence << ") :";
				for (const auto& [key, signature] : value)
					CATAPULT_LOG(debug) << "[DBRB] CONVERGED UPDATE: - " << key << " : " << signature;
			}

			auto mapSize = map.size();
			auto quorumSize = message.ReplacedView.quorumSize();
			bool triggered = (mapSize == quorumSize);
			CATAPULT_LOG(debug) << "[DBRB] CONVERGED: Quorum status is " << mapSize << "/" << quorumSize << (triggered ? " (TRIGGERED)" : " (NOT triggered)");

			return triggered;
		};

		std::set<View> update(const StateUpdateMessage& message) {
			std::set<View> triggeredViews;
			for (auto& [quorumKey, messages] : StateUpdateMessages) {
				const auto& view = quorumKey.Sequence.data().front();
				if (view.isMember(message.Sender)) {
					messages.insert(message);

					auto messagesCount = messages.size();
					auto quorumSize = view.quorumSize();
					bool triggered = (messagesCount == view.quorumSize());
					CATAPULT_LOG(debug) << "[DBRB] STATE-UPDATE: Quorum status is " << messagesCount << "/" << quorumSize << (triggered ? " (TRIGGERED)" : " (NOT triggered)");

					if (triggered)
						triggeredViews.insert(view);
				}
			}
			return triggeredViews;
		};

		bool update(AcknowledgedMessage message) {
			CATAPULT_LOG(debug) << "[DBRB] QUORUM: Received ACKNOWLEDGED message in view " << message.View << ".";
			auto& set = AcknowledgedPayloads[message.View];
			const auto& payloadHash = message.PayloadHash;
			set.emplace(message.Sender, payloadHash);

			const auto acknowledgedCount = std::count_if(set.begin(), set.end(), [&payloadHash](const auto& pair){ return pair.second == payloadHash; });

			const auto triggered = (acknowledgedCount == message.View.quorumSize());
			CATAPULT_LOG(debug) << "[DBRB] QUORUM: ACK quorum status is " << acknowledgedCount << "/" << message.View.quorumSize() << (triggered ? " (TRIGGERED)." : " (NOT triggered).");

			return triggered;
		};

		bool update(const DeliverMessage& message) {
			auto& set = DeliveredProcesses[message.View];
			if (set.emplace(message.Sender).second) {
				bool triggered = set.size() == message.View.quorumSize();
				CATAPULT_LOG(debug) << "[DBRB] QUORUM: DELIVER quorum status is " << set.size() << "/" << message.View.quorumSize() << (triggered ? " (TRIGGERED)." : " (NOT triggered).");
				return triggered;
			} else {
				return false;
			}
		};

	private:
		/// Updates a counter in \a map at \a key.
		/// Returns whether the quorum for \a referenceView has just been collected on this update.
		template<typename TKey>
		bool update(std::map<QuorumKey, std::set<ProcessId>>& map, const TKey& key, const ProcessId& id, size_t quorumSize, const std::string& name) {
			const auto quorumKey = QuorumKey(key);

			if (!map[quorumKey].insert(id).second) {
				CATAPULT_LOG(warning) << "[DBRB] " << name << " QUORUM: Not updated (process ID already exists). Quorum status is " << map.at(key).size() << "/" << quorumSize << ".";
				return false;
			}

			auto count = map.at(quorumKey).size();

			// Quorum collection is triggered only once, when the counter EXACTLY hits the quorum size.
			const auto triggered = (count == quorumSize);
			CATAPULT_LOG(warning) << "[DBRB] " << name << " QUORUM: Quorum status is " << count << "/" << quorumSize << (triggered ? " (TRIGGERED)." : " (NOT triggered).");

			return triggered;
		};
	};

	/// Class representing DBRB process.
	class DbrbProcess : public std::enable_shared_from_this<DbrbProcess>, public utils::NonCopyable {
	public:
		using DeliverCallback = consumer<const Payload&>;

	public:
		explicit DbrbProcess(
			std::shared_ptr<net::PacketWriters> pWriters,
			const net::PacketIoPickerContainer& packetIoPickers,
			ionet::Node thisNode,
			const crypto::KeyPair& keyPair,
			std::shared_ptr<thread::IoThreadPool> pPool,
			TransactionSender&& transactionSender);

	private:
		/// Process identifier.
		ProcessId m_id;

		/// This node.
		SignedNode m_node;

		/// Quorum manager.
		QuorumManager m_quorumManager;

		/// State of the process membership.
		MembershipState m_membershipState = MembershipState::NotJoined;

		/// Whether the process should keep disseminating Reconfig messages on discovery of new views.
		/// Set to true only when joining or leaving the system.
		bool m_disseminateReconfig = false;

		/// Whether the process should keep disseminating Commit messages on installing new views.
		/// Set to true only when leaving the system after an Install message.
		bool m_disseminateCommit = false;

		/// While set to true, no Prepare, Commit or Reconfig messages are processed.
		bool m_limitedProcessing = false;

		/// Most recent view known to the process.
		View m_currentView;

		/// Whether current view is installed by the process.
		bool m_currentViewIsInstalled = true;

		/// Needs to be set in order for \a onStateUpdateQuorumCollected to be triggered.
		std::optional<InstallMessageData> m_currentInstallMessage;

		/// List of pending changes (i.e., join or leave).
		View m_pendingChanges;

		/// Map that maps views to proposed sequences to replace those views.
		std::map<View, Sequence> m_proposedSequences;

		/// Map that maps views to the last converged sequence to replace those views.
		std::map<View, Sequence> m_lastConvergedSequences;

		struct BroadcastData {
			/// The sender of the data.
			ProcessId Sender;

			/// Payload allowed to be acknowledged. If empty, any payload can be acknowledged.
			dbrb::Payload Payload;

			/// Map that maps views and process IDs to signatures received from respective Acknowledged messages.
			std::map<std::pair<View, ProcessId>, Signature> Signatures;

			/// Message certificate; map that maps process IDs to signatures received from them.
			/// Empty when the process starts working.
			CertificateType Certificate;

			/// View in which message certificate was collected.
			View CertificateView;

			/// Quorum manager.
			dbrb::QuorumManager QuorumManager;

			/// Whether any commit message received.
			bool CommitMessageReceived = false;

			Timestamp Begin;
		};

		std::map<Hash256, BroadcastData> m_broadcastData;

		/// Whether there is any payload allowed to be acknowledged.
		bool m_acknowledgeAllowed = true;

		/// Whether process is allowed to leave the system.
		bool m_canLeave = false;

		/// State of the process.
		ProcessState m_state;

		NetworkPacketConverter m_converter;

		DeliverCallback m_deliverCallback;

		const crypto::KeyPair& m_keyPair;

		NodeRetreiver m_nodeRetreiver;

		std::shared_ptr<MessageSender> m_pMessageSender;

		boost::asio::io_context::strand m_strand;

		TransactionSender m_transactionSender;

	public:
		/// Request to leave the system.
		void leave();

		/// Broadcast arbitrary \c payload into the system.
		void broadcast(const Payload&);

		void processMessage(const Message& message);

		void clearBroadcastData();

	public:
		void registerPacketHandlers(ionet::ServerPacketHandlers& packetHandlers);
		void setDeliverCallback(const DeliverCallback& callback);
		void onViewDiscovered(const ViewData&);

		NodeRetreiver& nodeRetreiver();

	private:
		void disseminate(const std::shared_ptr<Message>& pMessage, std::set<ProcessId> recipients);
		void send(const std::shared_ptr<Message>& pMessage, const ProcessId& recipient);

		void prepareForStateUpdates(const InstallMessageData&);
		void updateState(const std::set<StateUpdateMessage>&);
		void extendPendingChanges(const View&);
		Signature sign(const Payload&);
		static bool verify(const ProcessId&, const Payload&, const View&, const Signature&);
		
		void onReconfigMessageReceived(const ReconfigMessage&);
		void onReconfigConfirmMessageReceived(const ReconfigConfirmMessage&);
		void onProposeMessageReceived(const ProposeMessage&);
		void onConvergedMessageReceived(const ConvergedMessage&);
		void onInstallMessageReceived(const InstallMessage&);

		void onPrepareMessageReceived(const PrepareMessage&);
		void onStateUpdateMessageReceived(const StateUpdateMessage&);
		void onAcknowledgedMessageReceived(const AcknowledgedMessage&);
		void onCommitMessageReceived(const CommitMessage&);
		void onDeliverMessageReceived(const DeliverMessage&);

		void onReconfigConfirmQuorumCollected();
		void onProposeQuorumCollected(const ProposeMessage&);
		void onConvergedQuorumCollected(const ConvergedMessage&);
		void onStateUpdateQuorumCollected();
		void onAcknowledgedQuorumCollected(const AcknowledgedMessage&);
		void onDeliverQuorumCollected(const Payload&, const ProcessId&);

		void onViewInstalled(const View&);
		void onLeaveAllowed();
		void onJoinComplete();
		void onLeaveComplete();
	};
}}