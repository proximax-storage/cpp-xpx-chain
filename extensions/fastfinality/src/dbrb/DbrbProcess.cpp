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
		m_currentView.Data.emplace(m_id, MembershipChanges::Join);
		for (const auto& node : bootstrapNodes) {
			m_currentView.Data.emplace(node, MembershipChanges::Join);
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
		// Can only request to join from the initial membership state of the process.
		if (m_membershipState != MembershipStates::NotJoined)
			return;

		m_membershipState = MembershipStates::Joining;

		// Resetting counters for ReconfigConfirm messages to get a proper quorum collection event later.
		m_quorumManager.ReconfigConfirmCounters.clear();

		// Somehow, while this flag is true, a view discovery protocol should be working
		// that would call onViewDiscovered() whenever it gets a new view of the system.
		m_viewDiscoveryActive = true;

		// Keep disseminating new Reconfig messages whenever a new view is discovered.
		m_disseminateReconfig = true;
	}

	void DbrbProcess::leave() {
		// Can only request to leave when the process is participating in the system.
		if (m_membershipState != MembershipStates::Participating)
			return;

		const auto& pAcknowledgeable = m_state.Acknowledgeable;
		const bool processIsSender = pAcknowledgeable.has_value() && pAcknowledgeable->Sender == m_id;
		bool needsPermissionToLeave = m_payloadIsDelivered || processIsSender;
		if (needsPermissionToLeave && !m_canLeave)
			return;	// TODO: Can notify user about the reason why leave failed

		m_membershipState = MembershipStates::Leaving;

		// Disseminate a Reconfig message for every currently installed view...
		for (const auto& view : m_installedViews) {
			ReconfigMessage message(m_id, m_id, MembershipChanges::Leave, view);
			disseminate(message, view.members());
		}

		// ...and keep disseminating new Reconfig messages whenever a new view is installed.
		m_disseminateReconfig = true;
	}

	void DbrbProcess::broadcast(const Payload& payload) {
		if (!m_installedViews.count(m_currentView))
			return;	// The process waits to install some view and then disseminates the prepare message.
					// TODO: Can notify user about the reason why broadcast failed

		PrepareMessage message(m_id, payload, m_currentView);
		disseminate(message, message.View.members());
	}

	void DbrbProcess::processMessage(const Message&) { /* Identifies message type and calls respective private method. */ }


	// Basic private methods:

	void DbrbProcess::disseminate(const Message& message, const std::set<ProcessId>& recipients) {
		for (const auto& recipient : recipients)
			send(message, recipient);
	}

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

	void DbrbProcess::updateState(const std::set<StateUpdateMessage>& messages) {
		std::map<Payload, std::set<PrepareMessage>> payloads;	// Maps different payloads to sets of Prepare messages
																// that contain those payloads.
		std::set<CommitMessage> storedCommits;

		for (const auto& message : messages) {
			// TODO: Validate signature of the messages, continue/skip if invalid

			// Updating payloads.
			const auto& pAcknowledgeable = message.State.Acknowledgeable;
			if (pAcknowledgeable.has_value()) {
				auto& set = payloads[pAcknowledgeable->Payload];
				set.emplace(*pAcknowledgeable);
				if (payloads.size() > 1)
					break;	// At least two different acknowledgeable payloads exist among states.
			}

			const auto& pConflicting = message.State.Conflicting;
			if (pConflicting.has_value()) {
				payloads.clear();
				payloads[pConflicting->first.Payload] = { pConflicting->first };
				payloads[pConflicting->second.Payload] = { pConflicting->second };
				break;	// Some process has provided two conflicting Prepare messages.
			}

			// Updating storedCommits.
			const auto& pStored = message.State.Stored;
			if(pStored.has_value()) {
				const auto& certificate = pStored->Certificate;
				const auto& certificateView = pStored->CertificateView;
				const auto& payload = pStored->Payload;

				bool certificateValid = true;
				for (const auto& [sender, signature] : certificate) {
					const bool signatureValid = verify(sender, payload, certificateView, signature);
					if (!signatureValid) {
						certificateValid = false;
						break;
					}
				}

				if (certificateValid)
					storedCommits.emplace(*pStored);
			}
		}

		bool canUpdateAcknowledgeable = m_acknowledgeAllowed && !payloads.empty();
		if (canUpdateAcknowledgeable
				&& m_acknowledgeablePayload.has_value()
				&& *m_acknowledgeablePayload != payloads.begin()->first) {
			canUpdateAcknowledgeable = false;
		}

		if (payloads.size() == 1 && canUpdateAcknowledgeable) {
			m_acknowledgeablePayload = payloads.begin()->first;
			if (!m_state.Acknowledgeable.has_value()) {
				const auto& prepareMessages = payloads.begin()->second;
				const auto& firstPrepareMessage = *prepareMessages.begin();
				m_state.Acknowledgeable = firstPrepareMessage;
			}
		} else if (payloads.size() > 1 && !payloads.empty()) {
			m_acknowledgeAllowed = false;
			m_state.Acknowledgeable.reset();
			if (!m_state.Conflicting.has_value()) {
				auto pPayloads = payloads.begin();
				const auto& prepareMessagesA = pPayloads->second;
				const auto& prepareMessagesB = (++pPayloads)->second;
				const auto& firstPrepareMessageA = *prepareMessagesA.begin();
				const auto& firstPrepareMessageB = *prepareMessagesB.begin();
				m_state.Conflicting = std::make_pair(firstPrepareMessageA, firstPrepareMessageB);
			}
		}

		if (!m_storedPayloadData.has_value() && !storedCommits.empty()) {
			const auto& firstCommitMessage = *storedCommits.begin();
			m_storedPayloadData = { firstCommitMessage.Payload,
									firstCommitMessage.Certificate,
									firstCommitMessage.CertificateView };
			if (!m_state.Stored.has_value()) {
				m_state.Stored = firstCommitMessage;
			}
		}
	}

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
		return {};	// TODO
	};

	bool DbrbProcess::verify(const ProcessId& signer, const Payload& payload, const View& view, const Signature& signature) {
		// Forms message body as described in DbrbProcess::sign and checks whether the signature is valid.
		return true;	// TODO
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
		if (message.MembershipChange == MembershipChanges::Leave) {
			const auto joinChange = std::make_pair(message.ProcessId, MembershipChanges::Join);
			if (!message.View.hasChange(message.ProcessId, MembershipChanges::Join))
				return;
		}

		m_pendingChanges.Data.emplace(message.ProcessId, message.MembershipChange);
		ReconfigConfirmMessage responseMessage(m_id, message.View);
		send(responseMessage, sender);
	}

	void DbrbProcess::onReconfigConfirmMessageReceived(const ReconfigConfirmMessage& message) {
		bool quorumCollected = m_quorumManager.update(message);
		if (quorumCollected)
			onReconfigConfirmQuorumCollected();
	}

	void DbrbProcess::onReconfigConfirmQuorumCollected() {
		if (m_membershipState == MembershipStates::Joining || m_membershipState == MembershipStates::Leaving)
			m_disseminateReconfig = false;

//		// TODO: Not clear if view discovery should remain active or not
//		if (m_membershipState == MembershipStates::Joining)
//			m_viewDiscoveryActive = false;
	}

	void DbrbProcess::onProposeMessageReceived(const ProposeMessage& message) {
		// Must be sent from a member of replaced view.
		if (!message.ReplacedView.isMember(message.Sender))
			return;

		// Filtering incorrect proposals.
		const auto& format = m_formatSequences[message.ReplacedView];
		if ( !(format.count(message.ProposedSequence) || format.count(Sequence{})) )	// TODO: format must contain empty sequence, or be empty?
			return;

		// Every view in ProposedSequence must be more recent than the current view of the process.
		const auto pLeastRecentView = message.ProposedSequence.maybeLeastRecent();
		if ( !pLeastRecentView || !(m_currentView < *pLeastRecentView) )
			return;

		// TODO: Check that there is at least one view in ProposedSequence that the process is not aware of

		auto& currentSequence = m_proposedSequences[message.ReplacedView];
		bool conflicting = !currentSequence.canAppend(message.ProposedSequence);
		if (conflicting) {
			const auto localMostRecent = currentSequence.maybeMostRecent().value_or(View{});
			const auto proposedMostRecent = message.ProposedSequence.maybeMostRecent().value_or(View{});
			const auto mergedView = View::merge(localMostRecent, proposedMostRecent);

			auto lastConverged = m_lastConvergedSequences[message.ReplacedView];
			lastConverged.tryAppend(mergedView);

			currentSequence = lastConverged;
		} else {
			currentSequence.tryAppend(message.ProposedSequence);	// Will always succeed.
		}

		ProposeMessage responseMessage(m_id, currentSequence, message.ReplacedView);
		disseminate(responseMessage, message.ReplacedView.members());

		// Updating quorum counter for received Propose message.
		bool quorumCollected = m_quorumManager.update(message);
		if (quorumCollected)
			onProposeQuorumCollected(message);
	}

	void DbrbProcess::onProposeQuorumCollected(const ProposeMessage& message) {
		m_lastConvergedSequences[message.ReplacedView] = message.ProposedSequence;

		ConvergedMessage responseMessage(m_id, message.ProposedSequence, message.ReplacedView);
		disseminate(responseMessage, message.ReplacedView.members());
	}

	void DbrbProcess::onConvergedMessageReceived(const ConvergedMessage& message) {
		bool quorumCollected = m_quorumManager.update(message);
		if (quorumCollected)
			onConvergedQuorumCollected(message);
	}

	void DbrbProcess::onConvergedQuorumCollected(const ConvergedMessage& message) {
		const auto& leastRecentView = message.ConvergedSequence.maybeLeastRecent().value_or(View{});
		InstallMessage responseMessage(m_id, leastRecentView, message.ConvergedSequence, message.ReplacedView);

		std::set<ProcessId> replacedViewMembers = message.ReplacedView.members();
		std::set<ProcessId> leastRecentViewMembers = leastRecentView.members();
		std::set<ProcessId> recipientsUnion;
		std::set_union(replacedViewMembers.begin(), replacedViewMembers.end(),
			leastRecentViewMembers.begin(), leastRecentViewMembers.end(),
			std::inserter(recipientsUnion, recipientsUnion.begin()));

		disseminate(responseMessage, recipientsUnion);
	}

	void DbrbProcess::onInstallMessageReceived(const InstallMessage& message) {
		// Update Format sequences.
		auto& format = m_formatSequences[message.LeastRecentView];
		auto sequenceWithoutLeastRecent = message.ConvergedSequence;
		sequenceWithoutLeastRecent.tryErase(message.LeastRecentView);	// Will always succeed.
		format.insert(sequenceWithoutLeastRecent);

		if (message.ReplacedView.isMember(m_id)) {
			if (m_currentView < message.LeastRecentView)
				m_limitedProcessing = true;	// Stop processing Prepare, Commit and Reconfig messages.

			// TODO: Double-check; not clear what was meant by state(v)
			StateUpdateMessage stateUpdateMessage(m_id, m_state, message.ReplacedView, m_pendingChanges);

			std::set<ProcessId> replacedViewMembers = message.ReplacedView.members();
			std::set<ProcessId> leastRecentViewMembers = message.LeastRecentView.members();
			std::set<ProcessId> recipientsUnion;
			std::set_union(replacedViewMembers.begin(), replacedViewMembers.end(),
				leastRecentViewMembers.begin(), leastRecentViewMembers.end(),
				std::inserter(recipientsUnion, recipientsUnion.begin()));

			disseminate(stateUpdateMessage, recipientsUnion);
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
			auto& pAcknowledgeable = m_state.Acknowledgeable;
			if (!pAcknowledgeable.has_value())
				pAcknowledgeable = message;
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
		View reconfigRequests;
		for (const auto& message : stateUpdateMessages) {
			reconfigRequests.merge(message.PendingChanges);
		}
		reconfigRequests.difference(leastRecentView);
		m_pendingChanges.merge(reconfigRequests);

		// Uninstalling the least recent view mentioned in the current Install message.
		m_installedViews.erase(leastRecentView);

		// Updating state and payload-related fields of the process.
		updateState(stateUpdateMessages);

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
					&& m_currentView < moreRecentSequence.maybeLeastRecent().value_or(View{})) {
				m_proposedSequences[m_currentView] = moreRecentSequence;

				ProposeMessage proposeMessage(m_id, moreRecentSequence, m_currentView);
				disseminate(proposeMessage, m_currentView.members());
			} else {
				m_installedViews.insert(m_currentView);
				onViewInstalled(m_currentView);
			}

		} else {
			if (m_storedPayloadData.has_value()) {
				// Start disseminating Commit messages until allowed to leave.
				m_disseminateCommit = true;
				CommitMessage responseMessage(m_id, m_storedPayloadData->Payload, m_storedPayloadData->Certificate,
											  m_storedPayloadData->CertificateView, m_currentView);
				disseminate(responseMessage, m_currentView.members());
			} else {
				// Otherwise, can leave immediately.
				onLeaveComplete();
			}
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

	void DbrbProcess::onAcknowledgedQuorumCollected(const AcknowledgedMessage& message) {
		// Replacing view associated with the certificate.
		m_certificateView = message.View;

		// Replacing certificate.
		m_certificate.clear();
		const auto& acknowledgedSet = m_quorumManager.AcknowledgedPayloads[message.View];
		for (const auto& [processId, payload] : acknowledgedSet) {
			if (payload == message.Payload)
				m_certificate[processId] = m_signatures.at(std::make_pair(message.View, processId));
		}

		// Disseminating Commit message, if process' current view is installed.
		if (m_installedViews.count(m_currentView)) {
			CommitMessage responseMessage(m_id, message.Payload, m_certificate, m_certificateView, m_currentView);
			disseminate(responseMessage, m_currentView.members());
		}
	}

	void DbrbProcess::onCommitMessageReceived(const CommitMessage& message) {
		if (m_limitedProcessing)
			return;

		// View specified in the message must be equal to the current view of the process.
		if (message.CurrentView != m_currentView)
			return;

		// Message certificate must be valid, i.e. all signatures in it must be valid.
		for (const auto& [signer, signature] : message.Certificate) {
			if (!verify(signer, message.Payload, message.CertificateView, signature))
				return;
		}

		// Update stored PayloadData and ProcessState, if necessary,
		// and disseminate Commit message with updated view.
		if (!m_storedPayloadData.has_value()) {
			m_storedPayloadData = { message.Payload, message.Certificate, message.CertificateView };
			auto& pStored = m_state.Stored;
			if (!pStored.has_value()) {
				pStored = message;
			}

			CommitMessage responseMessage(m_id, message.Payload, message.Certificate, message.CertificateView, m_currentView);
			disseminate(responseMessage, m_currentView.members());
		}

		// Allow delivery for sender process.
		DeliverMessage deliverMessage(m_id, message.Payload, m_currentView);
		send(deliverMessage, message.Sender);
	}

	void DbrbProcess::onDeliverMessageReceived(const DeliverMessage& message) {
		// Message sender must be a member of the view specified in the message.
		if (!message.View.isMember(message.Sender))
			return;

		bool quorumCollected = m_quorumManager.update(message);
		if (quorumCollected)
			onDeliverQuorumCollected();
	}

	void DbrbProcess::onDeliverQuorumCollected() {
		m_payloadIsDelivered = true;

		if (m_storedPayloadData.has_value())	// Should always be set.
			m_deliverCallback(m_storedPayloadData->Payload);

		onLeaveAllowed();
	}


	// Other callbacks:

	void DbrbProcess::onViewDiscovered(View&& newView) {
		if (!m_viewDiscoveryActive)
			return;

		m_currentView = std::move(newView);

		if (m_membershipState == MembershipStates::Joining && m_disseminateReconfig) {
			ReconfigMessage message(m_id, m_id, MembershipChanges::Join, m_currentView);
			disseminate(message, m_currentView.members());
		}

		// If the process is leaving after an Install message.
		if (m_disseminateCommit) {
			// All fields in m_storedPayloadData are guaranteed to exist.
			CommitMessage responseMessage(m_id, m_storedPayloadData->Payload, m_storedPayloadData->Certificate,
										  m_storedPayloadData->CertificateView, m_currentView);
			disseminate(responseMessage, m_currentView.members());
		}
	}

	void DbrbProcess::onViewInstalled(const View& newView) {
		// Resume processing Prepare, Commit and Reconfig messages.
		m_limitedProcessing = false;

		if (!m_pendingChanges.Data.empty() && !m_proposedSequences.count(newView)) {
			const auto mergedView = View::merge(newView, m_pendingChanges);
			const auto sequence = Sequence::fromViews({ mergedView });	// Single view is always a valid sequence.
			m_proposedSequences[newView] = *sequence;

			ProposeMessage message(m_id, *sequence, newView);
			disseminate(message, newView.members());
		}

		const auto& pAcknowledgeable = m_state.Acknowledgeable;
		const bool processIsSender = pAcknowledgeable.has_value() && pAcknowledgeable->Sender == m_id;
		const bool certificateExists = !m_certificate.empty();

		if (processIsSender && !certificateExists) {
			PrepareMessage message(m_id, pAcknowledgeable->Payload, newView);
			disseminate(message, newView.members());
		}
		if (processIsSender && certificateExists && !m_canLeave) {
			// If m_certificate exists, then m_certificateView also exists.
			// TODO: Double-check which payload should be used.
			CommitMessage responseMessage(m_id, m_storedPayloadData->Payload, m_certificate, m_certificateView, newView);
			disseminate(responseMessage, newView.members());
		}
		if (!processIsSender && m_storedPayloadData.has_value() && !m_canLeave) {
			CommitMessage responseMessage(m_id, m_storedPayloadData->Payload, m_storedPayloadData->Certificate,
										  m_storedPayloadData->CertificateView, newView);
			disseminate(responseMessage, newView.members());
		}

		// Disseminate Reconfig message using the installed view, if the process is leaving.
		if (m_membershipState == MembershipStates::Leaving) {
			ReconfigMessage message(m_id, m_id, MembershipChanges::Leave, newView);
			disseminate(message, newView.members());
		}
	}

	void DbrbProcess::onLeaveAllowed() {
		m_canLeave = true;
		if (m_disseminateCommit)
			onLeaveComplete();
	}

	void DbrbProcess::onJoinComplete() {
		m_disseminateReconfig = false;
//		m_viewDiscoveryActive = false;	// TODO: Not clear if view discovery should remain active or not
		m_membershipState = MembershipStates::Participating;
	}

	void DbrbProcess::onLeaveComplete() {
		m_disseminateReconfig = false;
		m_disseminateCommit = false;
		m_viewDiscoveryActive = false;
		m_membershipState = MembershipStates::Left;
	}
}}