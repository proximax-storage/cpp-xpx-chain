/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma  once
#include "catapult/types.h"
#include "DbrbUtils.h"
#include "Messages.h"
#include "MessageSender.h"
#include "catapult/functions.h"
#include "catapult/net/PacketWriters.h"
#include "catapult/ionet/PacketHandlers.h"

namespace catapult { namespace thread { class IoThreadPool; }}

namespace catapult { namespace dbrb {

	/// Struct that encapsulates all necessary quorum counters and their update methods.
	struct QuorumManager {
	public:
		/// Maps views to numbers of received ReconfigConfirm messages for those views.
		std::map<View, uint32_t> ReconfigConfirmCounters;

		/// Maps view-sequence pairs to numbers of received Proposed messages for those pairs.
		std::map<std::pair<View, Sequence>, uint32_t> ProposedCounters;

		/// Maps view-sequence pairs to processes with their signatures that received Converged messages for those pairs.
		std::map<std::pair<View, Sequence>, std::map<ProcessId, Signature>> ConvergedSignatures;

		/// Maps views to received StateUpdate messages for those views.
		std::map<View, std::set<StateUpdateMessage>> StateUpdateMessages;

		/// Maps views to sets of pairs of respective process IDs and payload hashes received from Acknowledged messages.
		std::map<View, std::set<std::pair<ProcessId, Hash256>>> AcknowledgedPayloads;

		/// Maps views to sets of process IDs ready for delivery.
		std::map<View, std::set<ProcessId>> DeliveredProcesses;

		/// Overloaded methods for updating respective counters.
		/// Returns whether the quorum has just been collected on this update.

		bool update(const ReconfigConfirmMessage& message) {
			CATAPULT_LOG(debug) << "[DBRB] QUORUM: Received RECONFIG CONFIRM message in view " << message.View << ".";
			return update(ReconfigConfirmCounters, message.View, message.View);
		};

		bool update(const ProposeMessage& message) {
			const auto keyPair = std::make_pair(message.ReplacedView, message.ProposedSequence);
			return update(ProposedCounters, keyPair, message.ReplacedView);
		};

		bool update(const ConvergedMessage& message) {
			const auto keyPair = std::make_pair(message.ReplacedView, message.ConvergedSequence);
			auto& map = ConvergedSignatures[keyPair];
			if (map.count(message.Sender)) {
				return false; // This process has already sent a Converged message; do nothing, quorum is NOT triggered on this message.
			}
			map[message.Sender] = message.Signature;
			return map.size() == message.ReplacedView.quorumSize();
		};

		std::set<View> update(const StateUpdateMessage& message) {
			std::set<View> triggeredViews;
			for (auto& [view, messages] : StateUpdateMessages) {
				if (view.isMember(message.Sender)) {
					messages.insert(message);
					if (messages.size() == view.quorumSize())
						triggeredViews.insert(view);
				}
			}
			return triggeredViews;
		};

		bool update(AcknowledgedMessage message) {
			CATAPULT_LOG(debug) << "[DBRB] QUORUM: Received ACKNOWLEDGED message in view " << message.View << ".";
			auto& set = AcknowledgedPayloads[message.View];
			auto payloadHash = CalculateHash({ { reinterpret_cast<const uint8_t*>(message.Payload.get()), message.Payload->Size } });
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
		template<typename Key>
		bool update(std::map<Key, uint32_t>& map, const Key& key, const View& referenceView) {
			auto count = ++map[key];

			// Quorum collection is triggered only once, when the counter EXACTLY hits the quorum size.
			const auto triggered = (count == referenceView.quorumSize());
			CATAPULT_LOG(debug) << "[DBRB] QUORUM: Quorum status is " << count << "/" << referenceView.quorumSize() << (triggered ? " (TRIGGERED)." : "(NOT triggered).");

			return triggered;
		};
	};

	/// Struct that encapsulates payload, its certificate and certificate view.
	struct PayloadData {
		/// Stored payload.
		dbrb::Payload Payload;

		/// Message certificate for the stored payload.
		CertificateType Certificate;

		/// View associated with the certificate.
		View CertificateView;
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
			std::shared_ptr<thread::IoThreadPool> pPool);

	private:
		/// Process identifier.
		ProcessId m_id;

		/// This node.
		SignedNode m_node;

		/// Quorum manager.
		QuorumManager m_quorumManager;

		/// State of the process membership.
		MembershipState m_membershipState = MembershipState::NotJoined;

		/// Whether the view discovery is active. View discovery halts once a quorum of
		/// confirmation messages has been collected for any of the views.
		bool m_viewDiscoveryActive = false;

		/// Whether the process should keep disseminating Reconfig messages on discovery of new views.
		/// Set to true only when joining or leaving the system.
		bool m_disseminateReconfig = false;

		/// Whether the process should keep disseminating Commit messages on installing new views.
		/// Set to true only when leaving the system after an Install message.
		bool m_disseminateCommit = false;

		/// While set to true, no Prepare, Commit or Reconfig messages are processed.
		bool m_limitedProcessing = false;

		/// Needs to be set in order for \a onStateUpdateQuorumCollected to be triggered.
		std::optional<InstallMessageData> m_currentInstallMessage;

		/// Views installed by the process.
		std::set<View> m_installedViews;

		/// Most recent view known to the process.
		View m_currentView;

		/// List of pending changes (i.e., join or leave).
		View m_pendingChanges;

		/// Map that maps views to proposed sequences to replace those views.
		std::map<View, Sequence> m_proposedSequences;

		/// Map that maps views to the last converged sequence to replace those views.
		std::map<View, Sequence> m_lastConvergedSequences;

		/// Map that maps views to the sets of sequences that can be proposed.
		std::map<View, std::set<Sequence>> m_formatSequences;

		struct BroadcastData {
			/// Payload allowed to be acknowledged. If empty, any payload can be acknowledged.
			std::optional<Payload> AcknowledgeablePayload;

			/// Map that maps views and process IDs to signatures received from respective Acknowledged messages.
			std::map<std::pair<View, ProcessId>, Signature> Signatures;

			/// Message certificate; map that maps process IDs to signatures received from them.
			/// Empty when the process starts working.
			CertificateType Certificate;

			/// View in which message certificate was collected.
			View CertificateView;

			/// Quorum manager.
			dbrb::QuorumManager QuorumManager;

			/// Stored payload, along with respective certificate and certificate view.
			/// Unset when the process starts working.
			std::optional<PayloadData> StoredPayloadData;

			/// Whether value of the payload is delivered.
			bool PayloadIsDelivered = false;
		};

		std::map<ProcessId, BroadcastData> m_broadcastData;

		/// Whether there is any payload allowed to be acknowledged.
		bool m_acknowledgeAllowed = true;

		/// Whether process is allowed to leave the system.
		bool m_canLeave = false;

		/// State of the process.
		ProcessState m_state;

		NetworkPacketConverter m_converter;

		DeliverCallback m_deliverCallback;

		const crypto::KeyPair& m_keyPair;

		mutable std::mutex m_mutex;

		NodeRetreiver m_nodeRetreiver;

		MessageSender m_messageSender;

		std::shared_ptr<thread::IoThreadPool> m_pPool;

	public:
		/// Request to join the system.
		void join();

		/// Request to leave the system.
		void leave();

		/// Broadcast arbitrary \c payload into the system.
		void broadcast(const Payload&);

		void processMessage(const Message& message);

		void clearBroadcastData();

	public:
		void registerPacketHandlers(ionet::ServerPacketHandlers& packetHandlers);
		void setDeliverCallback(const DeliverCallback& callback);
		void setCurrentView(const ViewData& viewData);

		const SignedNode& node();
		NodeRetreiver& nodeRetreiver();

	private:
		void disseminate(const std::shared_ptr<Message>& pMessage, std::set<ProcessId> recipients);
		void send(const std::shared_ptr<Message>& pMessage, const ProcessId& recipient);

		void prepareForStateUpdates(const InstallMessageData&);
		void updateState(const std::set<StateUpdateMessage>&);
		bool isAcknowledgeable(const PrepareMessage&);
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
		void onDeliverQuorumCollected(const std::optional<PayloadData>&);

		void onViewDiscovered(View&&);
		void onViewInstalled(const View&);
		void onLeaveAllowed();
		void onJoinComplete();
		void onLeaveComplete();
	};
}}