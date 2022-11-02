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
			m_viewDiscoveryActive = true;
		};

		void DbrbProcess::leave() {};

		void DbrbProcess::broadcast(Payload payload) {};

		void DbrbProcess::processMessage(Message message) { /* Identifies message type and calls respective private method. */ };


		// Basic private methods:

		void DbrbProcess::disseminate(
				Message message,
				std::set<ProcessId> recipients) { /* Sends message to all processes in recipients. */ };

		void DbrbProcess::reliablyDisseminate(
				Message message,
				std::set<ProcessId> recipients) { /* Reliably sends message to all processes in recipients. */ };

		void DbrbProcess::send(Message message, ProcessId recipient) {
			disseminate(message, { recipient });
		};

		void DbrbProcess::prepareForStateUpdates(InstallMessage message) {
			m_currentInstallMessage = message;
			m_quorumManager.StateUpdateMessages[message.ReplacedView] = {};	// TODO: May be redundant
		};

		void DbrbProcess::transferState(std::set<StateUpdateMessage>) {};


		// Message callbacks:

		void DbrbProcess::onReconfigMessageReceived(ReconfigMessage message) {
			if (m_limitedProcessing)
				return;

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
			bool quorumCollected = m_quorumManager.update(message);
			if (quorumCollected)
				onReconfigConfirmQuorumCollected();
		};

		void DbrbProcess::onReconfigConfirmQuorumCollected() {
			m_viewDiscoveryActive = false;
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
				currentSequence.tryAppend(*pLocalMostRecent);
				currentSequence.tryInsert(*pProposedMostRecent);
			} else {
				currentSequence.tryAppend(message.ProposedSequence);
			}

			ProposeMessage responseMessage { m_id, currentSequence, message.ReplacedView };
			disseminate(responseMessage, message.ReplacedView.members());

			// Updating quorum counter for received Propose message.
			bool quorumCollected = m_quorumManager.update(message);
			if (quorumCollected)
				onProposeQuorumCollected(message);
		};

		void DbrbProcess::onProposeQuorumCollected(ProposeMessage message) {
			m_lastConvergedSequences[message.ReplacedView] = message.ProposedSequence;

			ConvergedMessage responseMessage { m_id, message.ProposedSequence, message.ReplacedView };
			disseminate(responseMessage, message.ReplacedView.members());
		}

		void DbrbProcess::onConvergedMessageReceived(ConvergedMessage message) {
			bool quorumCollected = m_quorumManager.update(message);
			if (quorumCollected)
				onConvergedQuorumCollected(message);
		};

		void DbrbProcess::onConvergedQuorumCollected(ConvergedMessage message) {
			const auto& leastRecentView = *message.ConvergedSequence.maybeLeastRecent();	// Will always exist.
			InstallMessage responseMessage { m_id, leastRecentView, message.ConvergedSequence, message.ReplacedView };

			std::set<ProcessId> recipientsUnion;
			std::set_union(message.ReplacedView.Data.begin(), message.ReplacedView.Data.end(),
						   leastRecentView.Data.begin(), leastRecentView.Data.end(),
						   recipientsUnion.begin());

			reliablyDisseminate(responseMessage, recipientsUnion);
		};

		void DbrbProcess::onInstallMessageReceived(InstallMessage message) {
			// Update Format sequences.
			auto& format = m_formatSequences[message.LeastRecentView];
			auto sequenceWithoutLeastRecent = message.ConvergedSequence;
			sequenceWithoutLeastRecent.tryErase(message.LeastRecentView);
			format.insert(sequenceWithoutLeastRecent);

			if (message.ReplacedView.isMember(m_id)) {
				if (m_currentView < message.LeastRecentView)
					m_limitedProcessing = true;	// Stop processing Prepare, Commit and Reconfig messages.

				auto& state = m_states[message.ReplacedView];	// TODO: Check if this always exists
				StateUpdateMessage stateUpdateMessage { m_id, state, m_pendingChanges };

				std::set<ProcessId> recipientsUnion;
				std::set_union(message.ReplacedView.Data.begin(), message.ReplacedView.Data.end(),
							   message.LeastRecentView.Data.begin(), message.LeastRecentView.Data.end(),
							   recipientsUnion.begin());

				reliablyDisseminate(stateUpdateMessage, recipientsUnion);
			}

			if (m_currentView < message.LeastRecentView) {
				// Wait until a quorum of StateUpdate messages is collected for (message.ReplacedView).
				prepareForStateUpdates(message);
			}
		};

		void DbrbProcess::onPrepareMessageReceived(PrepareMessage message) {
			if (m_limitedProcessing)
				return;
		};

		void DbrbProcess::onStateUpdateMessageReceived(StateUpdateMessage message) {
			const auto triggeredViews = m_quorumManager.update(message);
			const bool quorumCollected =
					m_currentInstallMessage.has_value() && triggeredViews.count(m_currentInstallMessage->ReplacedView);
			if (quorumCollected)
				onStateUpdateQuorumCollected();
		};

		void DbrbProcess::onStateUpdateQuorumCollected() {
			const auto& stateUpdateMessages = m_quorumManager.StateUpdateMessages.at(m_currentInstallMessage->ReplacedView);
			const auto& leastRecentView = m_currentInstallMessage->LeastRecentView;
			const auto& convergedSequence = m_currentInstallMessage->ConvergedSequence;

			// Updating pending changes.
			// TODO: Change View.Data to set to be able to safely merge two lists of changes

			m_installedViews.erase(leastRecentView);

			transferState(stateUpdateMessages);

			if (leastRecentView.isMember(m_id)) {
				m_currentView = leastRecentView;

				if (!m_currentInstallMessage->ReplacedView.isMember(m_id))
					onJoinComplete();

				// Check if there are more recent views in the sequence of the install message.
				Sequence moreRecentSequence;
				for (auto iter = convergedSequence.data().rend(); m_currentView < *iter; ++iter)
					moreRecentSequence.tryInsert(*iter);

				if (!moreRecentSequence.data().empty()
						&& m_proposedSequences.count(m_currentView) == 0
						&& m_currentView < *moreRecentSequence.maybeLeastRecent()) {
					m_proposedSequences[m_currentView] = moreRecentSequence;

					ProposeMessage proposeMessage { m_id, moreRecentSequence, m_currentView };
					disseminate(proposeMessage, m_currentView.members());
				} else {
					m_installedViews.insert(m_currentView);
					m_limitedProcessing = false;	// Resume processing Prepare, Commit and Reconfig messages.
					onNewViewInstalled();
				}

			} else {
				if (m_payloadIsStored) {
					// TODO: Perform view discovery and disseminate Propose messages until is allowed to leave
				}

				onLeaveComplete();
			}

			// Installation is finished, resetting stored install message.
			m_currentInstallMessage.reset();
		};

		void DbrbProcess::onAcknowledgedMessageReceived(AcknowledgedMessage message) {};

		void DbrbProcess::onCommitMessageReceived(CommitMessage message) {
			if (m_limitedProcessing)
				return;
		};

		void DbrbProcess::onDeliverMessageReceived(DeliverMessage message) {};


		// Other callbacks:

		void DbrbProcess::onViewDiscovered(View newView) {
			if (!m_viewDiscoveryActive)
				return;

			m_currentView = std::move(newView);
			ReconfigMessage message { m_id, m_id, MembershipChanges::Join, m_currentView };
			disseminate(message, m_currentView.members());
		};

		void DbrbProcess::onJoinComplete() {};

		void DbrbProcess::onLeaveComplete() {};

		void DbrbProcess::onNewViewInstalled() {};

}}