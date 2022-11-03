/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma  once
#include "DbrbUtils.h"
#include "Messages.h"
#include "catapult/functions.h"
#include "catapult/net/PacketWriters.h"

namespace catapult { namespace fastfinality {

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


	/// Class representing DBRB process.
	class DbrbProcess {
	private:
		/// Process identifier.
		ProcessId m_id;

		/// Quorum manager.
		QuorumManager m_quorumManager;

		/// Whether the view discovery is active. View discovery halts once a quorum of
		/// confirmation messages has been collected for any of the views.
		bool m_viewDiscoveryActive;

		/// While set to true, no Prepare, Commit or Reconfig messages are processed.
		bool m_limitedProcessing;

		/// Needs to be set in order for \a onStateUpdateQuorumCollected to be triggered.
		std::optional<InstallMessage> m_currentInstallMessage;

		/// Views installed by the process.
		std::set<View> m_installedViews;	// TODO: Can be only one at a time?

		/// Most recent view known to the process.
		View m_currentView;

		/// List of pending changes (i.e., join or leave).
		std::vector<std::pair<ProcessId, MembershipChanges>> m_pendingChanges;

		/// Map that maps views to proposed sequences to replace those views.
		std::map<View, Sequence> m_proposedSequences;

		/// Map that maps views to the last converged sequence to replace those views.
		std::map<View, Sequence> m_lastConvergedSequences;

		/// Map that maps views to the sets of sequences that can be proposed.
		std::map<View, std::set<Sequence>> m_formatSequences;

		/// Message certificate for current payload.
		Certificate m_certificate;

		/// View in which message certificate was collected.
		View m_certificateView;

		/// List of payloads allowed to be acknowledged. If empty, any payload can be acknowledged.
		std::vector<Payload> m_acknowledgeAllowed;

		/// Value of the payload.
		Payload m_payload;

		/// Whether value of the payload is relevant.
		bool m_payloadIsStored;

		/// Whether value of the payload is delivered.
		bool m_payloadIsDelivered;

		/// Whether process is allowed to leave the system.
		bool m_canLeave;

		std::shared_ptr<net::PacketWriters> m_pWriters;

		/// Map that maps views to respective states.
		std::map<View, ProcessState> m_states;

	public:
		/// Request to join the system.
		void join();

		/// Request to leave the system.
		void leave();

		/// Broadcast arbitrary \c payload into the system.
		void broadcast(const Message&);

		void processMessage(const ionet::Packet&);

	private:
		using DisseminateCallback = consumer<ionet::SocketOperationCode, const ionet::Packet*>;

		void disseminate(const Message& message, const DisseminateCallback& callback);
		void reliablyDisseminate(Message, std::set<ProcessId>);	// R-multicast in terms of BRB.
		void send(const Message& message, const ionet::Node& recipient, const ionet::PacketIo::WriteCallback& callback);
		void prepareForStateUpdates(InstallMessage);
		void transferState(std::set<StateUpdateMessage>);

		// Request members of the current view forcibly remove failing participant.
		void forceLeave(const ionet::Node& node);

		void onReconfigMessageReceived(ReconfigMessage);
		void onReconfigConfirmMessageReceived(ReconfigConfirmMessage);
		void onReconfigConfirmQuorumCollected();
		void onProposeMessageReceived(ProposeMessage);
		void onProposeQuorumCollected(ProposeMessage);
		void onConvergedMessageReceived(ConvergedMessage);
		void onConvergedQuorumCollected(ConvergedMessage);
		void onInstallMessageReceived(InstallMessage);

		void onReconfigMessageReceived(const ReconfigMessage&, const ionet::Node& sender);
		void onReconfigConfirmMessageReceived(const ReconfigConfirmMessage&);
		void onProposeMessageReceived(const ProposeMessage&);
		void onConvergedMessageReceived(const ConvergedMessage&);
		void onInstallMessageReceived(const InstallMessage&);

		void onPrepareMessageReceived(const PrepareMessage& message);
		void onStateUpdateMessageReceived(const StateUpdateMessage&);
		void onStateUpdateQuorumCollected();
		void onAcknowledgedMessageReceived(const AcknowledgedMessage&);
		void onCommitMessageReceived(const CommitMessage&);
		void onDeliverMessageReceived(const DeliverMessage&);

		/// Called when a new (most recent) view of the system is discovered.
		void onViewDiscovered(View&&);

		void onJoinComplete();
		void onLeaveComplete();
		void onNewViewInstalled();
	};
}}