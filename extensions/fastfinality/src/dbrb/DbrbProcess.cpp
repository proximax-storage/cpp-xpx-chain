/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DbrbProcess.h"
#include "DbrbUtils.h"
#include "Messages.h"
#include "catapult/ionet/PacketPayload.h"
#include "catapult/thread/Future.h"
#include <utility>


namespace catapult { namespace dbrb {

	namespace {
		constexpr auto Default_Timeout = utils::TimeSpan::FromMinutes(1);

		constexpr utils::LogLevel MapToLogLevel(net::PeerConnectCode connectCode) {
			if (connectCode == net::PeerConnectCode::Accepted)
				return utils::LogLevel::Info;
			else
				return utils::LogLevel::Warning;
		}

		bool Connect(net::PacketWriters& writers, const ProcessId& node) {
			auto pPromise = std::make_shared<thread::promise<bool>>();
			writers.connect(node, [pPromise, node](const net::PeerConnectResult& result) {
				const auto& endPoint = node.endpoint();
				CATAPULT_LOG_LEVEL(MapToLogLevel(result.Code))
						<< "connection attempt to " << node << " @ " << endPoint.Host << " : " << endPoint.Port << " completed with " << result.Code;
				pPromise->set_value(result.Code == net::PeerConnectCode::Accepted);
			});

			return pPromise->get_future().get();
		}

		ionet::NodePacketIoPair GetNodePacketIoPair(net::PacketWriters& writers, const ProcessId& node) {
			auto nodePacketIoPair = writers.pickOne(Default_Timeout, node.identityKey());
			if (nodePacketIoPair.io())
				return nodePacketIoPair;

			bool success = Connect(writers, node);
			if (success)
				return writers.pickOne(Default_Timeout, node.identityKey());

			return {};
		}
	}

	DbrbProcess::DbrbProcess(
		std::shared_ptr<net::PacketWriters> pWriters,
		std::shared_ptr<ionet::ServerPacketHandlers> pPacketHandlers,
		const std::vector<ionet::Node>& bootstrapNodes,
		ionet::Node thisNode)
			: m_pWriters(std::move(pWriters))
			, m_pPacketHandlers(std::move(pPacketHandlers))
			, m_id(std::move(thisNode)) {
		m_currentView.Data.emplace_back(m_id, MembershipChanges::Join);
		for (const auto& node : bootstrapNodes) {
			m_currentView.Data.emplace_back(node, MembershipChanges::Join);
		}
	}

	void DbrbProcess::registerPacketHandlers() {
		auto handler = [pThis = shared_from_this(), &converter = m_converter](const auto& packet, auto& context) {
			auto pMessage = converter.toMessage(packet);
			pThis->processMessage(*pMessage);
		};
		m_pPacketHandlers->registerHandler(ionet::PacketType::Dbrb_Reconfig_Message, handler);
		m_pPacketHandlers->registerHandler(ionet::PacketType::Dbrb_Reconfig_Confirm_Message, handler);
		m_pPacketHandlers->registerHandler(ionet::PacketType::Dbrb_Propose_Message, handler);
		m_pPacketHandlers->registerHandler(ionet::PacketType::Dbrb_Converged_Message, handler);
		m_pPacketHandlers->registerHandler(ionet::PacketType::Dbrb_Install_Message, handler);
		m_pPacketHandlers->registerHandler(ionet::PacketType::Dbrb_Prepare_Message, handler);
		m_pPacketHandlers->registerHandler(ionet::PacketType::Dbrb_State_Update_Message, handler);
		m_pPacketHandlers->registerHandler(ionet::PacketType::Dbrb_Acknowledged_Message, handler);
		m_pPacketHandlers->registerHandler(ionet::PacketType::Dbrb_Commit_Message, handler);
		m_pPacketHandlers->registerHandler(ionet::PacketType::Dbrb_Deliver_Message, handler);
	}

	void DbrbProcess::setDeliverCallback(const DeliverCallback& callback) {
		m_deliverCallback = callback;
	}

	subscribers::NodeSubscriber& DbrbProcess::getNodeSubscriber() {
		return m_nodeSubscriber;
	}

	// Basic operations:

	void DbrbProcess::join() {
		// Somehow, while this flag is true, a view discovery protocol should be working
		// that would call onViewDiscovered() whenever it gets a new view of the system.
		m_viewDiscoveryActive = true;
	}

	void DbrbProcess::leave() {}

	void DbrbProcess::broadcast(const Payload& payload) {
		if (!m_installedViews.count(m_currentView))
			return;	// The process waits to install some view and then disseminates the prepare message.
					// TODO: Can notify user the reason why broadcast failed

		PrepareMessage message(m_id, payload, m_currentView);
		disseminate(message, message.View.members());
	}

	void DbrbProcess::processMessage(const Message&) { /* Identifies message type and calls respective private method. */ }


	// Basic private methods:

	void DbrbProcess::disseminate(const Message& message, const std::set<ProcessId>& recipients) {
		for (const auto& recipient : recipients)
			send(message, recipient);
	}

	void DbrbProcess::reliablyDisseminate(
		const Message& message,
		std::set<ProcessId> recipients) { /* Reliably sends message to all processes in recipients. */ }

	void DbrbProcess::send(const Message& message, const ProcessId& recipient) {
		auto nodePacketIoPair = GetNodePacketIoPair(*m_pWriters, recipient);
		if (nodePacketIoPair.io())
			nodePacketIoPair.io()->write(ionet::PacketPayload(message.toNetworkPacket()), [](ionet::SocketOperationCode){});
	}

	void DbrbProcess::forceLeave(const ProcessId& node) {
	}

	void DbrbProcess::prepareForStateUpdates(const InstallMessage& message) {
		m_currentInstallMessage = message;
		m_quorumManager.StateUpdateMessages[message.ReplacedView] = {};	// TODO: May be redundant
	}

	void DbrbProcess::transferState(const std::set<StateUpdateMessage>&) {}

	bool DbrbProcess::isAcknowledgeable(const Payload& payload) {
		// If m_acknowledgeAllowed == false, no payload is allowed to be acknowledged.
		if (!m_acknowledgeAllowed)
			return false;

		// If m_acknowledgeAllowed == true and m_acknowledgeablePayload is unset,
		// any payload can be acknowledged.
		if (!m_acknowledgeablePayload.has_value())
			return true;

		// If m_acknowledgeAllowed == true and m_acknowledgeablePayload is set,
		// only m_acknowledgeablePayload can be acknowledged.
		return *m_acknowledgeablePayload == payload;
	}

	Signature DbrbProcess::sign(const ProcessId& sender, const Payload& payload) {
		// Signed message must contain information about:
		// - sender process S from which Prepare message was received;
		// - payload of the message;
		// - recipient process Q (current process) that received Prepare message from S;
		// - Q's view at the moment of forming a signature;
	};

	bool DbrbProcess::verify(const ProcessId& signer, const Payload& payload, const View& view, const Signature& signature) {
		// Forms message body as described in DbrbProcess::sign and checks whether the signature is valid.
	};


	// Message callbacks:

	void DbrbProcess::onReconfigMessageReceived(const ReconfigMessage& message, const ProcessId& sender) {
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

		m_pendingChanges.Data.emplace_back(message.ProcessId, message.MembershipChange);
		ReconfigConfirmMessage responseMessage(m_id, message.View);
		send(responseMessage, sender);
	}

	void DbrbProcess::onReconfigConfirmMessageReceived(const ReconfigConfirmMessage& message) {
	bool quorumCollected = m_quorumManager.update(message);
		if (quorumCollected)
			onReconfigConfirmQuorumCollected();
	}

	void DbrbProcess::onReconfigConfirmQuorumCollected() {
		m_viewDiscoveryActive = false;
	}

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

		ProposeMessage responseMessage(m_id, currentSequence, message.ReplacedView);
		disseminate(responseMessage, m_currentView.members() );
		// Updating quorum counter for received Propose message.
		bool quorumCollected = m_quorumManager.update(message);
		if (quorumCollected)
			onProposeQuorumCollected(message);
	}

	void DbrbProcess::onProposeQuorumCollected(const ProposeMessage& message) {
		m_lastConvergedSequences[message.ReplacedView] = message.ProposedSequence;

		ConvergedMessage responseMessage(m_id, message.ProposedSequence, message.ReplacedView);
		disseminate(responseMessage, message.ReplacedView.members() );
	}

	void DbrbProcess::onConvergedMessageReceived(const ConvergedMessage& message) {
		bool quorumCollected = m_quorumManager.update(message);
		if (quorumCollected)
			onConvergedQuorumCollected(message);
	}

	void DbrbProcess::onConvergedQuorumCollected(const ConvergedMessage& message) {
		const auto& leastRecentView = *message.ConvergedSequence.maybeLeastRecent();	// Will always exist.
		InstallMessage responseMessage(m_id, leastRecentView, message.ConvergedSequence, message.ReplacedView);

		std::set<ProcessId> replacedViewMembers = message.ReplacedView.members();
		std::set<ProcessId> leastRecentViewMembers = leastRecentView.members();
		std::set<ProcessId> recipientsUnion;
		std::set_union(replacedViewMembers.begin(), replacedViewMembers.end(),
			leastRecentViewMembers.begin(), leastRecentViewMembers.end(),
			std::inserter(recipientsUnion, recipientsUnion.begin()));

		reliablyDisseminate(responseMessage, recipientsUnion);
	}

	void DbrbProcess::onInstallMessageReceived(const InstallMessage& message) {
		// Update Format sequences.
		auto& format = m_formatSequences[message.LeastRecentView];
		auto sequenceWithoutLeastRecent = message.ConvergedSequence;
		sequenceWithoutLeastRecent.tryErase(message.LeastRecentView);
		format.insert(sequenceWithoutLeastRecent);

		if (message.ReplacedView.isMember(m_id)) {
			if (m_currentView < message.LeastRecentView)
				m_limitedProcessing = true;	// Stop processing Prepare, Commit and Reconfig messages.

			const auto& state = m_state;	// TODO: Check if this always exists
			StateUpdateMessage stateUpdateMessage(m_id, state, m_pendingChanges);

			std::set<ProcessId> replacedViewMembers = message.ReplacedView.members();
			std::set<ProcessId> leastRecentViewMembers = message.LeastRecentView.members();
			std::set<ProcessId> recipientsUnion;
			std::set_union(replacedViewMembers.begin(), replacedViewMembers.end(),
				leastRecentViewMembers.begin(), leastRecentViewMembers.end(),
				std::inserter(recipientsUnion, recipientsUnion.begin()));

			reliablyDisseminate(stateUpdateMessage, recipientsUnion);
		}

		if (m_currentView < message.LeastRecentView) {
			// Wait until a quorum of StateUpdate messages is collected for (message.ReplacedView).
			prepareForStateUpdates(message);
		}
	}

	void DbrbProcess::onPrepareMessageReceived(const PrepareMessage& message) {
		if (m_limitedProcessing)
			return;
		// Message sender must be a member of the view specified in the message.
		if (!message.View.isMember(message.Sender))
			return;

		// View specified in the message must be equal to the current view of the process.
		if (message.View != m_currentView)
			return;

		// Payload from the message must be acknowledgeable.
		if (!isAcknowledgeable(message.Payload))
			return;

		if (!m_acknowledgeablePayload.has_value()) {
			m_acknowledgeablePayload = message.Payload;
			// TODO: Update State.ack to be <Prepare, m, v>
		}

		Signature signature = sign(message.Sender, message.Payload);
		AcknowledgedMessage responseMessage(m_id, message.Payload, m_currentView, signature);
		send(responseMessage, message.Sender);
	}

	void DbrbProcess::onStateUpdateMessageReceived(const StateUpdateMessage& message) {
		const auto triggeredViews = m_quorumManager.update(message);
		const bool quorumCollected =
				m_currentInstallMessage.has_value() && triggeredViews.count(m_currentInstallMessage->ReplacedView);
		if (quorumCollected)
			onStateUpdateQuorumCollected();
	}

	void DbrbProcess::onStateUpdateQuorumCollected() {
		const auto& stateUpdateMessages = m_quorumManager.StateUpdateMessages.at(m_currentInstallMessage->ReplacedView);
		const auto& leastRecentView = m_currentInstallMessage->LeastRecentView;
		const auto& convergedSequence = m_currentInstallMessage->ConvergedSequence;

		// Updating pending changes.
		// TODO: Merge two lists of changes

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

				ProposeMessage proposeMessage(m_id, moreRecentSequence, m_currentView);
				disseminate(proposeMessage, m_currentView.members() );
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
	}

	void DbrbProcess::onAcknowledgedMessageReceived(const AcknowledgedMessage& message) {
		// Message sender must be a member of the view specified in the message.
		if (!message.View.isMember(message.Sender))
			return;

		// There must not be an existing acknowledged entry from Sender for corresponding View.
		auto& acknowledgedSet = m_quorumManager.AcknowledgedPayloads[message.View];
		if (std::find_if(acknowledgedSet.begin(), acknowledgedSet.end(),
				[&message](const std::pair<ProcessId, Payload>& pair){ return pair.first == message.Sender; }) != acknowledgedSet.end())
			return;

		// Signature must be valid.
		if (!verify(message.Sender, message.Payload, message.View, message.Signature))
			return;

		m_signatures[std::make_pair(message.View, message.Sender)] = message.Signature;
		bool quorumCollected = m_quorumManager.update(message);
		if (quorumCollected && m_certificate.empty())
			onAcknowledgedQuorumCollected(message);
	};

	void DbrbProcess::onAcknowledgedQuorumCollected(AcknowledgedMessage message) {
		// Replacing certificate.
		m_certificate.clear();
		const auto& acknowledgedSet = m_quorumManager.AcknowledgedPayloads[message.View];
		for (const auto& [processId, payload] : acknowledgedSet) {
			if (payload == message.Payload)
				m_certificate.insert(m_signatures.at(std::make_pair(message.View, processId)));
		}

		// Replacing view associated with the certificate.
		m_certificateView = message.View;

		// Disseminating Commit message, if process' current view is installed.
		if (m_installedViews.count(m_currentView)) {
			CommitMessage responseMessage(m_id, message.Payload, m_certificate, m_certificateView, m_currentView);
			disseminate(responseMessage, m_currentView.members());
		}
	}

	void DbrbProcess::onCommitMessageReceived(const CommitMessage& message) {
		if (m_limitedProcessing)
			return;
	}

	void DbrbProcess::onDeliverMessageReceived(const DeliverMessage& message) {
		m_deliverCallback(message.Payload);
	}

	void DbrbProcess::onDeliverQuorumCollected(DeliverMessage message) {};


	// Other callbacks:

	void DbrbProcess::onViewDiscovered(View&& newView) {
		if (!m_viewDiscoveryActive)
			return;

		m_currentView = std::move(newView);
		ReconfigMessage message(m_id, m_id, MembershipChanges::Join, m_currentView);
		disseminate(message, m_currentView.members() );
	}

	void DbrbProcess::onJoinComplete() {}

	void DbrbProcess::onLeaveComplete() {}

	void DbrbProcess::onNewViewInstalled() {}
}}