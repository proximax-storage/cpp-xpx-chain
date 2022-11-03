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
		m_joinState.ViewDiscoveryActive = true;
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


	// Message callbacks:

	void DbrbProcess::onReconfigMessageReceived(const ReconfigMessage& message, const ionet::Node& sender) {
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
		m_joinState.addConfirm(message.View);
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
			auto newSequence = *(*localSequence.tryAppend(*pLocalMostRecent)).tryInsert(*pProposedMostRecent);

			currentSequence = std::move(newSequence);
		} else {
			currentSequence = *currentSequence.tryAppend(message.ProposedSequence);
		}

		ProposeMessage responseMessage;
		responseMessage.Sender = m_id;
		responseMessage.ProposedSequence = currentSequence;
		responseMessage.ReplacedView = message.ReplacedView;
		disseminate(responseMessage, [](ionet::SocketOperationCode, const ionet::Packet*) {});
	};

	void DbrbProcess::onConvergedMessageReceived(const ConvergedMessage& message) {};

	void DbrbProcess::onInstallMessageReceived(const InstallMessage& message) {};

	void DbrbProcess::onPrepareMessageReceived(const PrepareMessage& message) {};

	void DbrbProcess::onStateUpdateMessageReceived(const StateUpdateMessage& message) {};

	void DbrbProcess::onAcknowledgedMessageReceived(const AcknowledgedMessage& message) {};

	void DbrbProcess::onCommitMessageReceived(const CommitMessage& message) {};

	void DbrbProcess::onDeliverMessageReceived(const DeliverMessage& message) {};


	// Other callbacks:

	void DbrbProcess::onViewDiscovered(View&& newView) {
		if (!m_joinState.ViewDiscoveryActive)
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

	void DbrbProcess::onLeaveComplete() {};
}}