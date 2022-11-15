/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma  once
#include "catapult/types.h"
#include "DbrbUtils.h"
#include "Messages.h"
#include "catapult/functions.h"
#include "catapult/net/PacketWriters.h"
#include "catapult/ionet/PacketHandlers.h"
#include "catapult/subscribers/NodeSubscriber.h"

namespace catapult { namespace dbrb {

	/// Struct that encapsulates all necessary quorum counters and their update methods.
	struct QuorumManager {
	public:
		/// Maps views to numbers of received ReconfigConfirm messages for those views.
		std::map<View, uint32_t> ReconfigConfirmCounters;

		/// Maps view-sequence pairs to numbers of received Proposed messages for those pairs.
		std::map<std::pair<View, Sequence>, uint32_t> ProposedCounters;

		/// Maps view-sequence pairs to numbers of received Converged messages for those pairs.
		std::map<std::pair<View, Sequence>, uint32_t> ConvergedCounters;

		/// Maps views to received StateUpdate messages for those views.
		std::map<View, std::set<StateUpdateMessage>> StateUpdateMessages;

		/// Maps views to sets of pairs of respective process IDs and payloads received from Acknowledged messages.
		std::map<View, std::set< std::pair<ProcessId, Payload> >> AcknowledgedPayloads;

		/// Maps views to sets of process IDs ready for delivery.
		std::map<View, std::set<ProcessId>> DeliveredProcesses;

		/// Overloaded methods for updating respective counters.
		/// Returns whether the quorum has just been collected on this update.
		bool update(ReconfigConfirmMessage message) {
			return update(ReconfigConfirmCounters, message.View, message.View);
		};
		bool update(ProposeMessage message) {
			const auto keyPair = std::make_pair(message.ReplacedView, message.ProposedSequence);
			return update(ProposedCounters, keyPair, message.ReplacedView);
		};
		bool update(ConvergedMessage message) {
			const auto keyPair = std::make_pair(message.ReplacedView, message.ConvergedSequence);
			return update(ConvergedCounters, keyPair, message.ReplacedView);
		};
		std::set<View> update(StateUpdateMessage message) {
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
			auto& set = AcknowledgedPayloads[message.View];
			auto iter = std::find_if(set.begin(), set.end(),
					[&message](const std::pair<ProcessId, Payload>& pair){ return pair.first == message.Sender; });
			if (iter != set.end())
				set.erase(iter);
			set.emplace(message.Sender, message.Payload);

			const auto acknowledgedCount = std::count_if(set.begin(), set.end(),
					[&message](const std::pair<ProcessId, Payload>& pair){ return pair.second == message.Payload; });

			return acknowledgedCount == message.View.quorumSize();
		};
		bool update(DeliverMessage message) {
			auto& set = DeliveredProcesses[message.View];
			if (set.count(message.Sender)) {
				// Sender already was ready for delivery; set size is not updated, so quorum collection is not triggered.
				return false;
			}
			set.emplace(message.Sender);
			return set.size() == message.View.quorumSize();
		};

	private:
		/// Updates a counter in \a map at \a key.
		/// Returns whether the quorum for \a referenceView has just been collected on this update.
		template<typename Key>
		bool update(std::map<Key, uint32_t>& map, const Key& key, const View& referenceView) {
			if (map.count(key))
				map.at(key) += 1u;
			else
				map[key] = 1u;

			// Quorum collection is triggered only once, when the counter EXACTLY hits the quorum size.
			return map.at(key) == referenceView.quorumSize();
		};
	};

	class NoopNodeSubscriber : public subscribers::NodeSubscriber {
	public:
		void notifyNode(const ionet::Node&) {
		}

		void notifyIncomingNode(const Key&, ionet::ServiceIdentifier) {
		}
	};


	/// Struct that encapsulates payload, its certificate and certificate view.
	struct PayloadData {
		/// Stored payload.
		Payload Payload;

		/// Message certificate for the stored payload.
		std::map<ProcessId, Signature> Certificate;

		/// View associated with the certificate.
		View CertificateView;
	};


	/// Class representing DBRB process.
	class DbrbProcess : public std::enable_shared_from_this<DbrbProcess> {
	public:
		using DeliverCallback = consumer<const Payload&>;

	public:
		explicit DbrbProcess(
			std::shared_ptr<net::PacketWriters> pDbrbWriters,
			std::shared_ptr<net::PacketWriters> pClientWriters,
			std::shared_ptr<ionet::ServerPacketHandlers> pPacketHandlers,
			const std::vector<ionet::Node>& bootstrapNodes,
			ionet::Node thisNode);

	private:
		/// Process identifier.
		ProcessId m_id;

		/// Quorum manager.
		QuorumManager m_quorumManager;

		/// State of the process membership.
		MembershipStates m_membershipState = MembershipStates::NotJoined;

		/// Whether the view discovery is active. View discovery halts once a quorum of
		/// confirmation messages has been collected for any of the views.
		bool m_viewDiscoveryActive = false;

		/// Whether the process should keep disseminating Reconfig messages on updating or installing new views.
		/// Set to true only when joining or leaving the system.
		bool m_disseminateReconfig = false;

		/// While set to true, no Prepare, Commit or Reconfig messages are processed.
		bool m_limitedProcessing = false;

		/// Needs to be set in order for \a onStateUpdateQuorumCollected to be triggered.
		std::optional<InstallMessage> m_currentInstallMessage;

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

		/// Message certificate; map that maps process IDs to signatures received from them.
		/// Empty when the process starts working.
		std::map<ProcessId, Signature> m_certificate = {};

		/// View in which message certificate was collected.
		View m_certificateView;

		/// Map that maps views and process IDs to signatures received from respective Acknowledged messages.
		std::map<std::pair<View, ProcessId>, Signature> m_signatures;

		/// Whether there is any payload allowed to be acknowledged.
		bool m_acknowledgeAllowed = true;

		/// Payload allowed to be acknowledged. If empty, any payload can be acknowledged.
		std::optional<Payload> m_acknowledgeablePayload = {};

		/// Stored payload, along with respective certificate and certificate view.
		/// Unset when the process starts working.
		std::optional<PayloadData> m_storedPayloadData = {};

		/// Whether value of the payload is delivered.
		bool m_payloadIsDelivered = false;

		/// Whether process is allowed to leave the system.
		bool m_canLeave = false;

		/// State of the process.
		ProcessState m_state;

		std::shared_ptr<net::PacketWriters> m_pWriters;

		std::shared_ptr<net::PacketWriters> m_pClientWriters;

		std::shared_ptr<ionet::ServerPacketHandlers> m_pPacketHandlers;

		NetworkPacketConverter m_converter;

		DeliverCallback m_deliverCallback;

		NoopNodeSubscriber m_nodeSubscriber;

	public:
		/// Request to join the system.
		void join();

		/// Request to leave the system.
		void leave();

		/// Broadcast arbitrary \c payload into the system.
		void broadcast(const Payload&);

		void processMessage(const Message&);

	public:
		void setDeliverCallback(const DeliverCallback& callbacke);

		subscribers::NodeSubscriber& getNodeSubscriber();

		std::vector<ionet::NodePacketIoPair> getNodePacketIoPairs();

	private:
		void disseminate(const Message& message, const std::set<ProcessId>& recipients);
		void send(const Message& message, const ProcessId& recipient);

		void prepareForStateUpdates(const InstallMessage&);
		void transferState(const std::set<StateUpdateMessage>&);
		bool isAcknowledgeable(const Payload&);
		Signature sign(const ProcessId&, const Payload&);
		bool verify(const ProcessId&, const Payload&, const View&, const Signature&);

		// Request members of the current view forcibly remove failing participant.
		void forceLeave(const ProcessId& node);

		void onReconfigMessageReceived(const ReconfigMessage&, const ProcessId& sender);
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
		void onDeliverQuorumCollected();

		void onViewDiscovered(View&&);
		void onViewInstalled(const View&);

		void onJoinComplete();
		void onLeaveComplete();
	};
}}