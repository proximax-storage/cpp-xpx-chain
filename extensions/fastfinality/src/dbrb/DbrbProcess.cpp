#pragma  once
#include "DbrbProcess.h"

#include <utility>
#include "DbrbUtils.h"
#include "Messages.h"


namespace catapult { namespace fastfinality {

		// Basic operations:

		void DbrbProcess::join() {
			// Somehow, while this flag is true, a view discovery protocol should be working
			// that would call onViewDiscovered() whenever it gets a new view of the system.
			m_joinState.ViewDiscoveryActive = true;
		};

		void DbrbProcess::leave() {};

		void DbrbProcess::broadcast(Payload payload) {};

		void DbrbProcess::processMessage(Message message) { /* Identifies message type and calls respective private method. */ };


		// Basic private methods:

		void DbrbProcess::disseminate(
				Message message,
				std::set<ProcessId> recipients) { /* Sends message to all processes in recipients. */ };

		void DbrbProcess::send(Message message, ProcessId recipient) {
			disseminate(message, { recipient });
		};


		// Message callbacks:

		void DbrbProcess::onReconfigMessageReceived(ReconfigMessage message) {
			if (message.View != m_currentView)
				return;

			// Requested change must not be present in the view of the message.
			const auto& changes = message.View.Data;
			const auto requestedChange = std::make_pair(message.ProcessId, message.MembershipChange);
			if (std::find(changes.begin(), changes.end(), requestedChange) != changes.end())
				return;

			// When trying to leave, a corresponding join change must be present in the view of the message.
			const auto joinChange = std::make_pair(message.ProcessId, MembershipChanges::Join);
			if (std::find(changes.begin(), changes.end(), joinChange) == changes.end())
				return;

			m_pendingChanges.push_back(requestedChange);
			ReconfigConfirmMessage responseMessage { m_id, message.View };
			send(responseMessage, message.ProcessId);
		};

		void DbrbProcess::onReconfigConfirmMessageReceived(ReconfigConfirmMessage message) {
			m_joinState.addConfirm(message.View);
		};

		void DbrbProcess::onProposeMessageReceived(ProposeMessage message) {
			// Must be sent from a member of replaced view.
			if ( !message.ReplacedView.members().count(message.Sender) )
				return;

			// Filtering incorrect proposals.
			const auto& format = m_formatSequences.at(message.ReplacedView);
			// TODO: format must contain empty sequence, or be empty?
			if ( !(format.count(message.ProposedSequence) || format.count(Sequence{})) )
				return;

			// Every view in ProposedSequence must be more recent than the current view of the process.
			const auto pLeastRecentView = message.ProposedSequence.maybeLeastRecent();
			if ( !pLeastRecentView || !(m_currentView < *pLeastRecentView) )
				return;

			// TODO: Check that there is at least one view in ProposedSequence that the process is not aware of

			auto& currentSequence = m_proposedSequences.at(message.ReplacedView);
			bool conflicting = !currentSequence.canAppend(message.ProposedSequence);	// TODO: Double-check
			if (conflicting) {
				auto& localSequence = m_lastConvergedSequences.at(message.ReplacedView);
				const auto pLocalMostRecent = localSequence.maybeMostRecent();
				const auto pProposedMostRecent = message.ProposedSequence.maybeMostRecent();

				// We don't know which one of the views is more recent, so we first append one of them, which will
				// always succeed, and then we insert the other one. It is possible to use insert for both,
				// but it is better to use append whenever possible, as it is faster.
				auto newSequence = *(*localSequence.tryAppend(*pLocalMostRecent)).tryInsert(*pProposedMostRecent);

				currentSequence = std::move(newSequence);
			} else {
				currentSequence = *currentSequence.tryAppend(message.ProposedSequence);
			}

			ProposeMessage responseMessage { m_id, currentSequence, message.ReplacedView };
			disseminate(responseMessage, message.ReplacedView.members());
		};

		void DbrbProcess::onConvergedMessageReceived(ConvergedMessage message) {};

		void DbrbProcess::onInstallMessageReceived(InstallMessage message) {};

		void DbrbProcess::onPrepareMessageReceived(PrepareMessage message) {};

		void DbrbProcess::onStateUpdateMessageReceived(StateUpdateMessage message) {};

		void DbrbProcess::onAcknowledgedMessageReceived(AcknowledgedMessage message) {};

		void DbrbProcess::onCommitMessageReceived(CommitMessage message) {};

		void DbrbProcess::onDeliverMessageReceived(DeliverMessage message) {};


		// Other callbacks:

		void DbrbProcess::onViewDiscovered(View newView) {
			if (!m_joinState.ViewDiscoveryActive)
				return;

			m_currentView = std::move(newView);
			ReconfigMessage message { m_id, m_id, MembershipChanges::Join, m_currentView };
			disseminate(message, m_currentView.members());
		};

		void DbrbProcess::onJoinComplete() {};

		void DbrbProcess::onLeaveComplete() {};

}}