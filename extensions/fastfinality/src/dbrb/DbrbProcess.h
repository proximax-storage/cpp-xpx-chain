#pragma  once
#include "DbrbUtils.h"
#include "Messages.h"


namespace catapult { namespace fastfinality {

		/// Struct with all necessary fields needed for joining procedure.
		struct JoinState {
			/// Whether the view discovery is active. View discovery halts once a quorum of
			/// confirmation messages has been collected for any of the views.
			bool ViewDiscoveryActive = false;

			/// Maps views to numbers of received confirmations for those views.
			std::map<View, uint32_t> ViewConfirmations;

			/// Increments (or initializes) a counter of received confirms for a given \c view.
			/// Returns whether the view discovery is active after the counter is updated.
			bool addConfirm(View view) {
				if (std::find(ViewConfirmations.begin(), ViewConfirmations.end(), view) == ViewConfirmations.end())
					ViewConfirmations.emplace(view, 1u);
				else
					ViewConfirmations.at(view) += 1u;

				if (ViewConfirmations.at(view) >= view.quorumSize())
					ViewDiscoveryActive = false;

				return ViewDiscoveryActive;
			};
		};


		/// Class representing DBRB process.
		class DbrbProcess {
			/// Process identifier.
			ProcessId m_id;

			/// Information about joining procedure.
			JoinState m_joinState;

			/// Most recent view known to the process.
			View m_currentView;

			/// List of pending changes (i.e., join or leave).
			std::vector<std::pair<ProcessId, MembershipChanges>> m_pendingChanges;

			/// Map that maps views to proposed sequences to replace those views.
			std::map<View, Sequence> m_proposedSequences;

			/// Maps view-sequence pair to numbers of received Proposed messages for those pairs.
			std::map<std::pair<View, Sequence>, uint32_t> m_convergedCandidateSequences;

			/// Maps view-sequence pair to numbers of received Converged messages for those pairs.
			std::map<std::pair<View, Sequence>, uint32_t> m_installCandidateSequences;

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
			void onProposeMessageReceived(ProposeMessage);
			void onConvergedMessageReceived(ConvergedMessage);
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