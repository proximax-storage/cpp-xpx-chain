/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DbrbProcess.h"
#include "DbrbUtils.h"
#include "Messages.h"
#include "catapult/ionet/PacketPayload.h"
#include "catapult/thread/Future.h"
#include <utility>


namespace catapult { namespace fastfinality {

	namespace {
		constexpr auto Default_Timeout = utils::TimeSpan::FromMinutes(1);

		bool Connect(net::PacketWriters& writers, const ionet::Node& node) {
			auto pPromise = std::make_shared<thread::promise<bool>>();
			writers.connect(node, [pPromise](const net::PeerConnectResult& result) {
				pPromise->set_value(result.Code == net::PeerConnectCode::Accepted);
			});

			return pPromise->get_future().get();
		}

		ionet::NodePacketIoPair GetNodePacketIoPair(net::PacketWriters& writers, const ionet::Node& node) {
			auto nodePacketIoPair = writers.pickOne(Default_Timeout, node.identityKey());
			if (!!nodePacketIoPair.io())
				return nodePacketIoPair;

			bool success = Connect(writers, node);
			if (success)
				return writers.pickOne(Default_Timeout, node.identityKey());

			return {};
		}
	}

	// Basic operations:

	void DbrbProcess::join() {
		// Somehow, while this flag is true, a view discovery protocol should be working
		// that would call onViewDiscovered() whenever it gets a new view of the system.
		m_viewDiscoveryActive = true;
	};

	void DbrbProcess::leave() {};

	void DbrbProcess::broadcast(const Message&) {};

	void DbrbProcess::processMessage(const ionet::Packet&) { /* Identifies message type and calls respective private method. */ };


	// Basic private methods:

	void DbrbProcess::disseminate(const Message& message, const DisseminateCallback& callback) {
		auto recipients = m_currentView.members();
		for (const auto& recipient : recipients) {
			auto nodePacketIoPair = m_pWriters->pickOne(Default_Timeout, recipient.identityKey());
			if (!nodePacketIoPair.io()) {
				forceLeave(recipient);
				return;
			}

			nodePacketIoPair.io()->write(ionet::PacketPayload(message.packet()), [callback, &io = *nodePacketIoPair.io()](auto code) {
				if (ionet::SocketOperationCode::Success != code)
					return callback(code, nullptr);

				io.read(callback);
			});
		}
	};

	void DbrbProcess::reliablyDisseminate(
		Message message,
		std::set<ProcessId> recipients) { /* Reliably sends message to all processes in recipients. */ };

	void DbrbProcess::send(const Message& message, const ionet::Node& recipient, const ionet::PacketIo::WriteCallback& callback) {
		auto nodePacketIoPair = GetNodePacketIoPair(*m_pWriters, recipient);
		if (!nodePacketIoPair.io()) {
			forceLeave(recipient);
			callback(ionet::SocketOperationCode::Closed);
			return;
		}

		nodePacketIoPair.io()->write(ionet::PacketPayload(message.packet()), callback);
	};

	void DbrbProcess::forceLeave(const ionet::Node& node) {
	}

	void DbrbProcess::prepareForStateUpdates(InstallMessage message) {
		m_currentInstallMessage = message;
		m_quorumManager.StateUpdateMessages[message.ReplacedView] = {};	// TODO: May be redundant
	};

	void DbrbProcess::transferState(std::set<StateUpdateMessage>) {};


	// Message callbacks:

	void DbrbProcess::onReconfigMessageReceived(const ReconfigMessage& message, const ionet::Node& sender) {
		if (m_limitedProcessing)
				return;

		if (message.View != m_currentView)
			return;

		// Requested change must not be present in the view of the message.
		if (message.View.hasChange(message.ProcessId, message.MembershipChange))
			return;

		// When trying to leave, a corresponding join change must be present in the view of the message.
		const auto joinChange = std::make_pair(message.ProcessId, MembershipChanges::Join);
		if (!message.View.hasChange(message.ProcessId, MembershipChanges::Join))
			return;

		m_pendingChanges.emplace_back(message.ProcessId, message.MembershipChange);
		ReconfigConfirmMessage responseMessage;
		responseMessage.Sender = m_id;
		responseMessage.View = message.View;
		send(responseMessage, sender, [](ionet::SocketOperationCode) {});
	};

	void DbrbProcess::onReconfigConfirmMessageReceived(const ReconfigConfirmMessage& message) {
		bool quorumCollected = m_quorumManager.update(message);
			if (quorumCollected)
				onReconfigConfirmQuorumCollected();
		};

		void DbrbProcess::onReconfigConfirmQuorumCollected() {
			m_viewDiscoveryActive = false;
	};

	void DbrbProcess::onProposeMessageReceived(const ProposeMessage& message) {
		// Must be sent from a member of replaced view.
		auto replacedViewMembers = message.ReplacedView.members();
		if (!message.ReplacedView.isMember(message.Sender))
			return;

		// Filtering incorrect proposals.
//		const auto& format = m_formatSequences.at(message.ReplacedView);
//		// TODO: format must contain empty sequence, or be empty?
//		if ( !(format.count(message.ProposedSequence) || format.count(Sequence{})) )
//			return;

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

		ProposeMessage responseMessage;
		responseMessage.Sender = m_id;
		responseMessage.ProposedSequence = currentSequence;
		responseMessage.ReplacedView = message.ReplacedView;
		disseminate(responseMessage, [](ionet::SocketOperationCode, const ionet::Packet*) {});
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

	void DbrbProcess::onConvergedQuorumCollected(const ConvergedMessage& message) {
		const auto& leastRecentView = *message.ConvergedSequence.maybeLeastRecent();	// Will always exist.
		InstallMessage responseMessage { m_id, leastRecentView, message.ConvergedSequence, message.ReplacedView };

		std::set<ProcessId> recipientsUnion;
		std::set_union(message.ReplacedView.Data.begin(), message.ReplacedView.Data.end(),
					   leastRecentView.Data.begin(), leastRecentView.Data.end(),
					   recipientsUnion.begin());

		reliablyDisseminate(responseMessage, recipientsUnion);
	};

	void DbrbProcess::onInstallMessageReceived(const InstallMessage& message) {
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

	void DbrbProcess::onStateUpdateMessageReceived(const StateUpdateMessage& message) {
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

	void DbrbProcess::onAcknowledgedMessageReceived(const AcknowledgedMessage& message) {};

	void DbrbProcess::onCommitMessageReceived(const CommitMessage& message) {
		if (m_limitedProcessing)
			return;
	};

	void DbrbProcess::onDeliverMessageReceived(const DeliverMessage& message) {};


	// Other callbacks:

	void DbrbProcess::onViewDiscovered(View&& newView) {
		if (!m_viewDiscoveryActive)
			return;

		m_currentView = std::move(newView);
		ReconfigMessage message;
		message.Sender = m_id;
		message.ProcessId = m_id;
		message.MembershipChange = MembershipChanges::Join;
		message.View = m_currentView;
		disseminate(message, [](ionet::SocketOperationCode, const ionet::Packet*) {});
	};

	void DbrbProcess::onJoinComplete() {};

	void DbrbProcess::onLeaveComplete() {};void DbrbProcess::onNewViewInstalled() {};
}}