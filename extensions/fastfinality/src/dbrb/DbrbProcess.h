#pragma  once
#include "DbrbUtils.h"
#include "Messages.h"


namespace catapult { namespace fastfinality {

		/// Struct that encapsulates all necessary quorum counters and their update methods.
		struct QuorumCounters {
		public:
			/// Maps views to numbers of received ReconfigConfirm messages for those views.
			std::map<View, uint32_t> ReconfigConfirmCounters;

			/// Maps view-sequence pairs to numbers of received Proposed messages for those pairs.
			std::map<std::pair<View, Sequence>, uint32_t> ProposedCounters;

			/// Maps view-sequence pairs to numbers of received Converged messages for those pairs.
			std::map<std::pair<View, Sequence>, uint32_t> ConvergedCounters;

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
			/// Process identifier.
			ProcessId m_id;

			/// Quorum counters for Reconfig, .
			QuorumCounters m_quorumCounters;

			/// Whether the view discovery is active. View discovery halts once a quorum of
			/// confirmation messages has been collected for any of the views.
			bool m_viewDiscoveryActive;

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

		public:
			/// Request to join the system.
			void join();

			/// Request to leave the system.
			void leave();

			/// Broadcast arbitrary \c payload into the system.
			void broadcast(Payload);

			void processMessage(Message);

		private:
			void disseminate(Message, std::set<ProcessId>);	// TODO: With this signature messages are getting upcasted,
															// and all important fields from messages get cut off
			void send(Message, ProcessId);

			void onReconfigMessageReceived(ReconfigMessage);
			void onReconfigConfirmMessageReceived(ReconfigConfirmMessage);
			void onReconfigConfirmQuorumCollected();
			void onProposeMessageReceived(ProposeMessage);
			void onProposeQuorumCollected(ProposeMessage);
			void onConvergedMessageReceived(ConvergedMessage);
			void onConvergedQuorumCollected(ConvergedMessage);
			void onInstallMessageReceived(InstallMessage);

			void onPrepareMessageReceived(PrepareMessage message);
			void onStateUpdateMessageReceived(StateUpdateMessage);
			void onAcknowledgedMessageReceived(AcknowledgedMessage);
			void onCommitMessageReceived(CommitMessage);
			void onDeliverMessageReceived(DeliverMessage);

			/// Called when a new (most recent) view of the system is discovered.
			void onViewDiscovered(View);

			void onJoinComplete();
			void onLeaveComplete();
		};

}}