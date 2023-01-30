/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DbrbProcess.h"
#include "DbrbUtils.h"
#include "Messages.h"
#include "catapult/crypto/Signer.h"
#include "catapult/ionet/NetworkNode.h"
#include "catapult/ionet/PacketPayload.h"
#include "catapult/thread/IoThreadPool.h"
#include "catapult/thread/Future.h"
#include <utility>


namespace catapult { namespace dbrb {

	DbrbProcess::DbrbProcess(
		std::shared_ptr<net::PacketWriters> pWriters,
		const net::PacketIoPickerContainer& packetIoPickers,
		ionet::Node thisNode,
		const crypto::KeyPair& keyPair,
		std::shared_ptr<thread::IoThreadPool> pPool,
		TransactionSender&& transactionSender)
			: m_id(thisNode.identityKey())
			, m_keyPair(keyPair)
			, m_nodeRetreiver(packetIoPickers, thisNode.metadata().NetworkIdentifier)
			, m_messageSender(std::move(pWriters), m_nodeRetreiver)
			, m_pPool(pPool)
			, m_transactionSender(std::move(transactionSender)) {
		m_node.Node = thisNode;
		auto pPackedNode = ionet::PackNode(thisNode);
		auto hash = CalculateHash({ { reinterpret_cast<const uint8_t*>(pPackedNode.get()), pPackedNode->Size } });
		crypto::Sign(m_keyPair, hash, m_node.Signature);
	}

	void DbrbProcess::registerPacketHandlers(ionet::ServerPacketHandlers& packetHandlers) {
		std::weak_ptr<DbrbProcess> pThisWeak = shared_from_this();
		auto handler = [pThisWeak, &converter = m_converter](const auto& packet, auto& context) {
			auto pThis = pThisWeak.lock();
			if (!pThis)
				return;

			const auto& messagePacket = static_cast<const MessagePacket&>(packet);
			auto pBuffer = messagePacket.payload();
			auto hash = CalculateHash(messagePacket.buffers());
			if (!crypto::Verify(messagePacket.Sender, hash, messagePacket.Signature))
				return;

			auto pMessage = converter.toMessage(packet);
			pThis->processMessage(*pMessage);
		};
		packetHandlers.registerHandler(ionet::PacketType::Dbrb_Reconfig_Message, handler);
		packetHandlers.registerHandler(ionet::PacketType::Dbrb_Reconfig_Confirm_Message, handler);
		packetHandlers.registerHandler(ionet::PacketType::Dbrb_Propose_Message, handler);
		packetHandlers.registerHandler(ionet::PacketType::Dbrb_Converged_Message, handler);
		packetHandlers.registerHandler(ionet::PacketType::Dbrb_Install_Message, handler);
		packetHandlers.registerHandler(ionet::PacketType::Dbrb_Prepare_Message, handler);
		packetHandlers.registerHandler(ionet::PacketType::Dbrb_State_Update_Message, handler);
		packetHandlers.registerHandler(ionet::PacketType::Dbrb_Acknowledged_Message, handler);
		packetHandlers.registerHandler(ionet::PacketType::Dbrb_Commit_Message, handler);
		packetHandlers.registerHandler(ionet::PacketType::Dbrb_Deliver_Message, handler);
	}

	void DbrbProcess::setDeliverCallback(const DeliverCallback& callback) {
		m_deliverCallback = callback;
	}

	void DbrbProcess::setCurrentView(const ViewData& viewData) {
		m_currentView = View{ viewData };
		m_installedViews.emplace(m_currentView);

		std::set<ProcessId> ids;
		bool notJoined = true;
		for (const auto& [id, _] : viewData) {
			ids.emplace(id);

			if (m_id == id)
				notJoined = false;
		}
		m_nodeRetreiver.enqueue(ids);

		if (notJoined)
			join();
	}

	const SignedNode& DbrbProcess::node() {
		return m_node;
	}

	NodeRetreiver& DbrbProcess::nodeRetreiver() {
		return m_nodeRetreiver;
	}

	// Basic operations:

	void DbrbProcess::join() {
		CATAPULT_LOG(debug) << "[DBRB] JOIN initiated.";
		std::lock_guard<std::mutex> guard(m_mutex);

		// Can only request to join from the initial membership state of the process.
		if (m_membershipState != MembershipState::NotJoined)
			return;

		m_membershipState = MembershipState::Joining;

		// Resetting counters for ReconfigConfirm messages to get a proper quorum collection event later.
		m_quorumManager.ReconfigConfirmCounters.clear();

		// Somehow, while this flag is true, a view discovery protocol should be working
		// that would call onViewDiscovered() whenever it gets a new view of the system.
		m_viewDiscoveryActive = true;

		// Keep disseminating new Reconfig messages whenever a new view is discovered.
		m_disseminateReconfig = true;
	}

	void DbrbProcess::leave() {
		std::lock_guard<std::mutex> guard(m_mutex);

		// Can only request to leave when the process is participating in the system.
		if (m_membershipState != MembershipState::Participating)
			return;

//		const auto& pAcknowledgeable = m_state.Acknowledgeable;
//		const bool processIsSender = pAcknowledgeable.has_value() && pAcknowledgeable->Sender == m_id;
//		bool needsPermissionToLeave = m_payloadIsDelivered || processIsSender;
//		if (needsPermissionToLeave && !m_canLeave)
//			return;	// TODO: Can notify user about the reason why leave failed

		m_membershipState = MembershipState::Leaving;

		// Disseminate a Reconfig message for every currently installed view...
		for (const auto& view : m_installedViews) {
			auto pMessage = std::make_shared<ReconfigMessage>(m_id, m_id, MembershipChange::Leave, view);
			disseminate(pMessage, view.members());
		}

		// ...and keep disseminating new Reconfig messages whenever a new view is installed.
		m_disseminateReconfig = true;
	}

	void DbrbProcess::broadcast(const Payload& payload) {
		std::lock_guard<std::mutex> guard(m_mutex);

		// TODO: fix view search in set.
		if (!m_installedViews.count(m_currentView)) {
			CATAPULT_LOG(debug) << "[DBRB] BROADCAST: Current view is not installed, aborting broadcast.";
			return; // The process waits to install some view and then disseminates the prepare message.
					// TODO: Can notify user about the reason why broadcast failed
		}

		auto pMessage = std::make_shared<PrepareMessage>(m_id, payload, m_currentView);
		disseminate(pMessage, pMessage->View.members());
	}

	void DbrbProcess::processMessage(const Message& message) {
		std::lock_guard<std::mutex> guard(m_mutex);

		switch (message.Type) {
			case ionet::PacketType::Dbrb_Reconfig_Message: {
				CATAPULT_LOG(debug) << "[DBRB] Received RECONFIG message from " << message.Sender << ".";
				onReconfigMessageReceived(dynamic_cast<const ReconfigMessage&>(message));
				break;
			}
			case ionet::PacketType::Dbrb_Reconfig_Confirm_Message: {
				CATAPULT_LOG(debug) << "[DBRB] Received RECONFIG-CONFIRM message from " << message.Sender << ".";
				onReconfigConfirmMessageReceived(dynamic_cast<const ReconfigConfirmMessage&>(message));
				break;
			}
			case ionet::PacketType::Dbrb_Propose_Message: {
				CATAPULT_LOG(debug) << "[DBRB] Received PROPOSE message from " << message.Sender << ".";
				onProposeMessageReceived(dynamic_cast<const ProposeMessage&>(message));
				break;
			}
			case ionet::PacketType::Dbrb_Converged_Message: {
				CATAPULT_LOG(debug) << "[DBRB] Received CONVERGED message from " << message.Sender << ".";
				onConvergedMessageReceived(dynamic_cast<const ConvergedMessage&>(message));
				break;
			}
			case ionet::PacketType::Dbrb_Install_Message: {
				CATAPULT_LOG(debug) << "[DBRB] Received INSTALL message from " << message.Sender << ".";
				onInstallMessageReceived(dynamic_cast<const InstallMessage&>(message));
				break;
			}
			case ionet::PacketType::Dbrb_Prepare_Message: {
				CATAPULT_LOG(debug) << "[DBRB] Received PREPARE message from " << message.Sender << ".";
				onPrepareMessageReceived(dynamic_cast<const PrepareMessage&>(message));
				break;
			}
			case ionet::PacketType::Dbrb_State_Update_Message: {
				CATAPULT_LOG(debug) << "[DBRB] Received STATE-UPDATE message from " << message.Sender << ".";
				onStateUpdateMessageReceived(dynamic_cast<const StateUpdateMessage&>(message));
				break;
			}
			case ionet::PacketType::Dbrb_Acknowledged_Message: {
				CATAPULT_LOG(debug) << "[DBRB] Received ACKNOWLEDGED message from " << message.Sender << ".";
				onAcknowledgedMessageReceived(dynamic_cast<const AcknowledgedMessage&>(message));
				break;
			}
			case ionet::PacketType::Dbrb_Commit_Message: {
				CATAPULT_LOG(debug) << "[DBRB] Received COMMIT message from " << message.Sender << ".";
				onCommitMessageReceived(dynamic_cast<const CommitMessage&>(message));
				break;
			}
			case ionet::PacketType::Dbrb_Deliver_Message: {
				CATAPULT_LOG(debug) << "[DBRB] Received DELIVER message from " << message.Sender << ".";
				onDeliverMessageReceived(dynamic_cast<const DeliverMessage&>(message));
				break;
			}
			default:
				CATAPULT_THROW_RUNTIME_ERROR_1("invalid DBRB message type", message.Type)
		}
	}

	void DbrbProcess::clearBroadcastData() {
		m_broadcastData.clear();
		m_messageSender.clearQueue();
	}

	// Basic private methods:

	void DbrbProcess::disseminate(const std::shared_ptr<Message>& pMessage, std::set<ProcessId> recipients) {
		for (auto iter = recipients.begin(); iter != recipients.end(); ++iter) {
			if (m_id == *iter) {
				boost::asio::post(m_pPool->ioContext(), [pThis = shared_from_this(), pMessage]() {
					pThis->processMessage(*pMessage);
				});
				recipients.erase(iter);
				break;
			}
		}

		m_messageSender.enqueue(pMessage->toNetworkPacket(&m_keyPair), recipients);
	}

	void DbrbProcess::send(const std::shared_ptr<Message>& pMessage, const ProcessId& recipient) {
		if (m_id == recipient) {
			boost::asio::post(m_pPool->ioContext(), [pThis = shared_from_this(), pMessage]() {
				pThis->processMessage(*pMessage);
			});
		} else {
			m_messageSender.enqueue(pMessage->toNetworkPacket(&m_keyPair), std::set<ProcessId>{ recipient });
		}
	}

	void DbrbProcess::prepareForStateUpdates(const InstallMessageData& installMessageData) {
		m_currentInstallMessage = installMessageData;
		m_quorumManager.StateUpdateMessages[installMessageData.ReplacedView] = {};	// TODO: May be redundant
	}

	void DbrbProcess::updateState(const std::set<StateUpdateMessage>& messages) {
		std::map<Hash256, std::set<PrepareMessage>> payloads;	// Maps different payloads to sets of Prepare messages
																// that contain those payloads.
		std::set<CommitMessage> storedCommits;

		for (const auto& message : messages) {
			// TODO: Validate signature of the messages, continue/skip if invalid

			// Updating payloads.
			const auto& pAcknowledgeable = message.State.Acknowledgeable;
			Hash256 acknowledgeablePayloadHash;
			if (pAcknowledgeable.has_value()) {
				acknowledgeablePayloadHash = CalculatePayloadHash(pAcknowledgeable->Payload);
				payloads[acknowledgeablePayloadHash].emplace(*pAcknowledgeable);
				if (payloads.size() > 1)
					break;	// At least two different acknowledgeable payloads exist among states.
			}

			const auto& pConflicting = message.State.Conflicting;
			if (pConflicting.has_value()) {
				payloads.clear();
				auto conflictingPayloadHash = CalculatePayloadHash(pConflicting->Payload);
				payloads[acknowledgeablePayloadHash] = { *pAcknowledgeable };
				payloads[conflictingPayloadHash] = { *pConflicting };
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

//		bool canUpdateAcknowledgeable = m_acknowledgeAllowed && !payloads.empty();
//		if (canUpdateAcknowledgeable && m_acknowledgeablePayload.has_value() && CalculatePayloadHash(*m_acknowledgeablePayload) != payloads.begin()->first)
//			canUpdateAcknowledgeable = false;
//
//		if (payloads.size() == 1 && canUpdateAcknowledgeable) {
//			m_acknowledgeablePayload = payloads.begin()->second.begin()->Payload;
//			if (!m_state.Acknowledgeable.has_value()) {
//				const auto& prepareMessages = payloads.begin()->second;
//				const auto& firstPrepareMessage = *prepareMessages.begin();
//				m_state.Acknowledgeable = firstPrepareMessage;
//			}
//		} else if (payloads.size() > 1) {
//			m_acknowledgeAllowed = false;
//			m_state.Acknowledgeable.reset();
//			if (!m_state.Conflicting.has_value())
//				m_state.Conflicting = *(++payloads.begin())->second.begin();
//		}
//
//		if (!m_storedPayloadData.has_value() && !storedCommits.empty()) {
//			const auto& firstCommitMessage = *storedCommits.begin();
//			m_storedPayloadData = { firstCommitMessage.Payload,
//									firstCommitMessage.Certificate,
//									firstCommitMessage.CertificateView };
//			if (!m_state.Stored.has_value()) {
//				m_state.Stored = firstCommitMessage;
//			}
//		}
	}

	void DbrbProcess::extendPendingChanges(const View& appendedChanges) {
		m_pendingChanges.merge(appendedChanges);

		if (m_installedViews.count(m_currentView)) {
			// If there is no proposed sequence to replace current view, create one.
			if (!m_proposedSequences.count(m_currentView)) {
				auto newView = m_currentView;
				newView.merge(m_pendingChanges);
				const std::vector<View> sequenceData{ newView };

				m_proposedSequences[m_currentView] = Sequence::fromViews(sequenceData).value();

				CATAPULT_LOG(debug) << "[DBRB] Disseminating Propose message (pending changes have been extended).";
				auto pMessage = std::make_shared<ProposeMessage>(m_id, m_proposedSequences.at(m_currentView), m_currentView);
				disseminate(pMessage, m_currentView.members());
			}
		}
	}

	bool DbrbProcess::isAcknowledgeable(const PrepareMessage& message) {
		// If m_acknowledgeAllowed == false, no payload is allowed to be acknowledged.
		if (!m_acknowledgeAllowed) {
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: Not acknowledged (no payload is allowed).";
			return false;
		}

		// If m_acknowledgeAllowed == true and AcknowledgeablePayload is unset, any payload can be acknowledged.
		if (!m_broadcastData[message.Sender].AcknowledgeablePayload.has_value()) {
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: Acknowledged (any payload is allowed).";
			return true;
		}

		// If m_acknowledgeAllowed == true and AcknowledgeablePayload is set, only AcknowledgeablePayload can be acknowledged.
		const bool acknowledgeable = (CalculatePayloadHash(*m_broadcastData[message.Sender].AcknowledgeablePayload) == CalculatePayloadHash(message.Payload));
		if (acknowledgeable)
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: Acknowledged (matches stored acknowledgeable payload).";

		return acknowledgeable;
	}

	Signature DbrbProcess::sign(const Payload& payload) {
		// Signed message must contain information about:
		// - payload of the message;
		// - recipient process Q (current process) that received Prepare message from S;
		// - Q's view at the moment of forming a signature;
		// Hash calculated from concatenated information is signed.

		uint32_t payloadSize = m_currentView.packedSize();
		auto pPacket = ionet::CreateSharedPacket<ionet::Packet>(payloadSize);
		auto pBuffer = pPacket->Data();
		Write(pBuffer, m_currentView);

		auto hash = CalculateHash({ { reinterpret_cast<const uint8_t*>(payload.get()), payload->Size }, { pPacket->Data(), payloadSize } });
		Signature signature;
		crypto::Sign(m_keyPair, hash, signature);

		return signature;
	}

	bool DbrbProcess::verify(const ProcessId& signer, const Payload& payload, const View& view, const Signature& signature) {
		// Forms hash as described in DbrbProcess::sign and checks whether the signature is valid.

		uint32_t payloadSize = view.packedSize();
		auto pPacket = ionet::CreateSharedPacket<ionet::Packet>(payloadSize);
		auto pBuffer = pPacket->Data();
		Write(pBuffer, view);

		auto hash = CalculateHash({ { reinterpret_cast<const uint8_t*>(payload.get()), payload->Size }, { pPacket->Data(), payloadSize } });

		bool res = crypto::Verify(signer, hash, signature);
		return res;
	}


	// Message callbacks:

	void DbrbProcess::onReconfigMessageReceived(const ReconfigMessage& message) {
		if (m_limitedProcessing) {
			CATAPULT_LOG(debug) << "[DBRB] RECONFIG: Aborting message processing (limited processing is enabled).";
			return;
		}

		if (message.View != m_currentView) {
			CATAPULT_LOG(debug) << "[DBRB] RECONFIG: Aborting message processing (supplied view " << message.View << " doesn't match current view " << m_currentView << ").";
			return;
		}

		// Requested change must not be present in the view of the message.
		if (message.View.hasChange(message.ProcessId, message.MembershipChange)) {
			const auto changeStr = MembershipChangeToString(message.ProcessId, message.MembershipChange);
			CATAPULT_LOG(debug) << "[DBRB] RECONFIG: Aborting message processing (proposed change " << changeStr << " is already in current view " << m_currentView << ").";
			return;
		}

		// When trying to leave, a corresponding Join change must be present in the view of the message.
		if (message.MembershipChange == MembershipChange::Leave) {
			if (!message.View.hasChange(message.ProcessId, MembershipChange::Join)) {
				const auto changeStr = MembershipChangeToString(message.ProcessId, MembershipChange::Join);
				CATAPULT_LOG(debug) << "[DBRB] RECONFIG: Aborting message processing (no corresponding " << changeStr << " change in current view " << m_currentView << ").";
				return;
			}
		}

		View appendedView;
		appendedView.Data.emplace(message.ProcessId, message.MembershipChange);
		extendPendingChanges(appendedView);

		auto pMessage = std::make_shared<ReconfigConfirmMessage>(m_id, message.View);
		send(pMessage, message.Sender);
	}

	void DbrbProcess::onReconfigConfirmMessageReceived(const ReconfigConfirmMessage& message) {
		bool quorumCollected = m_quorumManager.update(message);
		if (quorumCollected)
			onReconfigConfirmQuorumCollected();
	}

	void DbrbProcess::onReconfigConfirmQuorumCollected() {
		CATAPULT_LOG(debug) << "[DBRB] RECONFIG-CONFIRM: Quorum collected.";
		if (m_membershipState == MembershipState::Joining || m_membershipState == MembershipState::Leaving) {
			CATAPULT_LOG(debug) << "[DBRB] RECONFIG-CONFIRM: Disabling Reconfig message dissemination (node is " << (m_membershipState == MembershipState::Joining ? "Joining" : "Leaving") << ").";
			m_disseminateReconfig = false;
		}


//		// TODO: Not clear if view discovery should remain active or not
//		if (m_membershipState == MembershipState::Joining)
//			m_viewDiscoveryActive = false;
	}

	void DbrbProcess::onProposeMessageReceived(const ProposeMessage& message) {
		// Must be sent from a member of replaced view.
		if (!message.ReplacedView.isMember(message.Sender))
			return;

		// Filtering incorrect proposals.
		const auto& format = m_formatSequences[message.ReplacedView];
		if ( !format.count(message.ProposedSequence) && !format.empty() )	// TODO: format must contain empty sequence, or be empty?
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

		CATAPULT_LOG(debug) << "[DBRB] PROPOSE: Disseminating Propose message.";
		auto pMessage = std::make_shared<ProposeMessage>(m_id, currentSequence, message.ReplacedView);
		disseminate(pMessage, message.ReplacedView.members());

		// Updating quorum counter for received Propose message.
		bool quorumCollected = m_quorumManager.update(message);
		if (quorumCollected)
			onProposeQuorumCollected(message);
	}

	void DbrbProcess::onProposeQuorumCollected(const ProposeMessage& message) {
		CATAPULT_LOG(debug) << "[DBRB] PROPOSE: Quorum collected in view " << message.ReplacedView << ".";
		m_lastConvergedSequences[message.ReplacedView] = message.ProposedSequence;

		CATAPULT_LOG(debug) << "[DBRB] PROPOSE: Disseminating Converged message.";
		auto pMessage = std::make_shared<ConvergedMessage>(m_id, message.ProposedSequence, message.ReplacedView);
		disseminate(pMessage, message.ReplacedView.members());
	}

	void DbrbProcess::onConvergedMessageReceived(const ConvergedMessage& message) {
		bool quorumCollected = m_quorumManager.update(message);
		if (quorumCollected)
			onConvergedQuorumCollected(message);
	}

	void DbrbProcess::onConvergedQuorumCollected(const ConvergedMessage& message) {
		CATAPULT_LOG(debug) << "[DBRB] CONVERGED: Quorum collected in view " << message.ReplacedView << ".";
		Sequence sequence;
		sequence.tryAppend(message.ReplacedView);
		sequence.tryAppend(message.ConvergedSequence);

		const auto keyPair = std::make_pair(message.ReplacedView, message.ConvergedSequence);
		const auto& convergedSignatures = m_quorumManager.ConvergedSignatures.at(keyPair);

		auto pMessage = std::make_shared<InstallMessage>(m_id, sequence, convergedSignatures);

		const auto& leastRecentView = message.ConvergedSequence.maybeLeastRecent().value_or(View{});
		std::set<ProcessId> replacedViewMembers = message.ReplacedView.members();
		std::set<ProcessId> leastRecentViewMembers = leastRecentView.members();
		std::set<ProcessId> recipientsUnion;
		std::set_union(replacedViewMembers.begin(), replacedViewMembers.end(),
			leastRecentViewMembers.begin(), leastRecentViewMembers.end(),
			std::inserter(recipientsUnion, recipientsUnion.begin()));

		CATAPULT_LOG(debug) << "[DBRB] CONVERGED: Disseminating Install message.";
		disseminate(pMessage, recipientsUnion);
	}

	void DbrbProcess::onInstallMessageReceived(const InstallMessage& message) {
		const auto pInstallMessageData = message.tryGetMessageData();
		if (!pInstallMessageData.has_value())
			return;	// TODO: Can inform client about ill-formed Install message

		const auto& replacedView = pInstallMessageData->ReplacedView;
		const auto& convergedSequence = pInstallMessageData->ConvergedSequence;
		const auto& leastRecentView = pInstallMessageData->LeastRecentView;

		// Update Format sequences.
		auto& format = m_formatSequences[leastRecentView];
		auto sequenceWithoutLeastRecent = convergedSequence;
		sequenceWithoutLeastRecent.tryErase(leastRecentView);	// Will always succeed.
		format.insert(sequenceWithoutLeastRecent);

		if (replacedView.isMember(m_id)) {
			if (m_currentView < leastRecentView)
				m_limitedProcessing = true;	// Stop processing Prepare, Commit and Reconfig messages.

			// TODO: Double-check; not clear what was meant by state(v)
			auto pMessage = std::make_shared<StateUpdateMessage>(m_id, m_state, replacedView, m_pendingChanges);

			std::set<ProcessId> replacedViewMembers = replacedView.members();
			std::set<ProcessId> leastRecentViewMembers = leastRecentView.members();
			std::set<ProcessId> recipientsUnion;
			std::set_union(replacedViewMembers.begin(), replacedViewMembers.end(),
				leastRecentViewMembers.begin(), leastRecentViewMembers.end(),
				std::inserter(recipientsUnion, recipientsUnion.begin()));

			disseminate(pMessage, recipientsUnion);
		}

		if (m_currentView < leastRecentView) {
			// Wait until a quorum of StateUpdate messages is collected for (message.ReplacedView).
			CATAPULT_LOG(debug) << "[DBRB] INSTALL: Preparing for state updates.";
			prepareForStateUpdates(*pInstallMessageData);
		}
	}

	void DbrbProcess::onPrepareMessageReceived(const PrepareMessage& message) {
		auto iter = m_broadcastData.find(message.Sender);
		if (iter != m_broadcastData.end() && iter->second.PayloadIsDelivered)
			m_broadcastData.erase(message.Sender);

		if (m_limitedProcessing) {
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: Aborting message processing (limited processing is enabled).";
			return;
		}

		// Message sender must be a member of the view specified in the message.
		if (!message.View.isMember(message.Sender)) {
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: Aborting message processing (sender is not in supplied view).";
			return;
		}

		// View specified in the message must be equal to the current view of the process.
		if (message.View != m_currentView) {
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: Aborting message processing (supplied view is not a current view).";
			return;
		}

		// Payload from the message must be acknowledgeable.
		if (!isAcknowledgeable(message)) {
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: Aborting message processing (payload is not acknowledgeable).";
			return;
		}

		auto& data = m_broadcastData[message.Sender];
		if (!data.AcknowledgeablePayload.has_value()) {
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: No acknowledgeable payload is stored.";
			data.AcknowledgeablePayload = message.Payload;
			if (!m_state.Acknowledgeable.has_value()) {
				CATAPULT_LOG(debug) << "[DBRB] PREPARE: Setting acknowledgeable payload to one from the message.";
				m_state.Acknowledgeable = message;
			}
		}

		CATAPULT_LOG(debug) << "[DBRB] PREPARE: Sending Acknowledged message to " << message.Sender << ".";
		Signature payloadSignature = sign(message.Payload);
		auto pMessage = std::make_shared<AcknowledgedMessage>(m_id, message.Sender, message.Payload, m_currentView, payloadSignature);
		send(pMessage, message.Sender);
	}

	void DbrbProcess::onStateUpdateMessageReceived(const StateUpdateMessage& message) {
		const auto triggeredViews = m_quorumManager.update(message);
		const bool quorumCollected = m_currentInstallMessage.has_value() && triggeredViews.count(m_currentInstallMessage->ReplacedView);
		if (quorumCollected)
			onStateUpdateQuorumCollected();
	}

	void DbrbProcess::onStateUpdateQuorumCollected() {
		CATAPULT_LOG(debug) << "[DBRB] STATE-UPDATE: Quorum collected in view " << m_currentInstallMessage->ReplacedView << ".";

		const auto& stateUpdateMessages = m_quorumManager.StateUpdateMessages.at(m_currentInstallMessage->ReplacedView);
		const auto& leastRecentView = m_currentInstallMessage->LeastRecentView;
		const auto& convergedSequence = m_currentInstallMessage->ConvergedSequence;

		// Updating pending changes.
		View reconfigRequests;
		for (const auto& message : stateUpdateMessages) {
			reconfigRequests.merge(message.PendingChanges);
		}
		reconfigRequests.difference(leastRecentView);
		extendPendingChanges(reconfigRequests);

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

				auto pMessage = std::make_shared<ProposeMessage>(m_id, moreRecentSequence, m_currentView);
				disseminate(pMessage, m_currentView.members());
			} else {
				m_installedViews.insert(m_currentView);

				Sequence sequence;
				sequence.tryAppend(m_currentInstallMessage->ReplacedView);
				sequence.tryAppend(m_currentInstallMessage->ConvergedSequence);
				InstallMessage installMessage(ProcessId(), std::move(sequence), m_currentInstallMessage->ConvergedSignatures);
				m_transactionSender.sendInstallMessageTransaction(installMessage);

				onViewInstalled(m_currentView);
			}

		} else {
			onLeaveComplete();
//			if (m_storedPayloadData.has_value()) {
//				// Start disseminating Commit messages until allowed to leave.
//				m_disseminateCommit = true;
//				auto pMessage = std::make_shared<CommitMessage>(m_id, m_storedPayloadData->Payload, m_storedPayloadData->Certificate, m_storedPayloadData->CertificateView, m_currentView);
//				disseminate(pMessage, m_currentView.members());
//			} else {
//				// Otherwise, can leave immediately.
//				onLeaveComplete();
//			}
		}

		// Installation is finished, resetting stored install message.
		m_currentInstallMessage.reset();
	}

	void DbrbProcess::onAcknowledgedMessageReceived(const AcknowledgedMessage& message) {
		// Message sender must be a member of the view specified in the message.
		if (!message.View.isMember(message.Sender)) {
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: Aborting message processing (sender is not in supplied view).";
			return;
		}

		// Signature must be valid.
		if (!verify(message.Sender, message.Payload, message.View, message.PayloadSignature)) {
			CATAPULT_LOG(warning) << "[DBRB] ACKNOWLEDGED: message REJECTED: signature is not valid";
			return;
		}

		auto& data = m_broadcastData[message.Initiator];
		data.Signatures[std::make_pair(message.View, message.Sender)] = message.PayloadSignature;
		bool quorumCollected = data.QuorumManager.update(message);
		if (quorumCollected && data.Certificate.empty())
			onAcknowledgedQuorumCollected(message);
	}

	void DbrbProcess::onAcknowledgedQuorumCollected(const AcknowledgedMessage& message) {
		CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: Quorum collected in view " << message.View << ".";
		// Replacing view associated with the certificate.
		CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: Setting CertificateView to " << message.View << ".";
		auto payloadHash = CalculatePayloadHash(message.Payload);

		// Replacing certificate.
		auto& data = m_broadcastData[message.Initiator];
		data.CertificateView = message.View;
		data.Certificate.clear();
		const auto& acknowledgedSet = data.QuorumManager.AcknowledgedPayloads[message.View];
		for (const auto& [processId, hash] : acknowledgedSet) {
			if (hash == payloadHash)
				data.Certificate[processId] = data.Signatures.at(std::make_pair(message.View, processId));
		}

		// Disseminating Commit message, if process' current view is installed.
		if (m_installedViews.count(m_currentView)) {
			CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: Disseminating Commit message.";
			auto pMessage = std::make_shared<CommitMessage>(m_id, message.Initiator, message.Payload, data.Certificate, data.CertificateView, m_currentView);
			disseminate(pMessage, m_currentView.members());
		} else {
			CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: Current view is not installed, Commit message is not disseminated.";
		}
	}

	void DbrbProcess::onCommitMessageReceived(const CommitMessage& message) {
		if (m_limitedProcessing) {
			CATAPULT_LOG(debug) << "[DBRB] COMMIT: Aborting message processing (limited processing is enabled).";
			return;
		}

		// View specified in the message must be equal to the current view of the process.
		if (message.CurrentView != m_currentView) {
			CATAPULT_LOG(debug) << "[DBRB] COMMIT: Aborting message processing (supplied view is not a current view).";
			return;
		}

		// Message certificate must be valid, i.e. all signatures in it must be valid.
		for (const auto& [signer, signature] : message.Certificate) {
			if (!verify(signer, message.Payload, message.CertificateView, signature)) {
				CATAPULT_LOG(warning) << "[DBRB] COMMIT: message REJECTED: signature is not valid";
				return;
			}
		}

		// Update stored PayloadData and ProcessState, if necessary,
		// and disseminate Commit message with updated view.
		auto& data = m_broadcastData[message.Initiator];
		if (!data.StoredPayloadData.has_value()) {
			data.StoredPayloadData = { message.Payload, message.Certificate, message.CertificateView };
			auto& pStored = m_state.Stored;
			if (!pStored.has_value())
				pStored = message;

			CATAPULT_LOG(debug) << "[DBRB] COMMIT: Disseminating Commit message.";
			auto pMessage = std::make_shared<CommitMessage>(m_id, message.Initiator, message.Payload, message.Certificate, message.CertificateView, m_currentView);
			disseminate(pMessage, m_currentView.members());
		}

		// Allow delivery for sender process.
		CATAPULT_LOG(debug) << "[DBRB] COMMIT: Sending Deliver message to " << message.Sender << ".";
		auto pMessage = std::make_shared<DeliverMessage>(m_id, message.Initiator, message.Payload, m_currentView);
		send(pMessage, message.Sender);
	}

	void DbrbProcess::onDeliverMessageReceived(const DeliverMessage& message) {
		// Message sender must be a member of the view specified in the message.
		if (!message.View.isMember(message.Sender)) {
			CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: Aborting message processing (sender is not in supplied view).";
			return;
		}

		auto& data = m_broadcastData[message.Initiator];
		bool quorumCollected = data.QuorumManager.update(message);
		if (quorumCollected) {
			onDeliverQuorumCollected(data.StoredPayloadData);
			data.PayloadIsDelivered = true;
		}
	}

	void DbrbProcess::onDeliverQuorumCollected(const std::optional<PayloadData>& storedPayloadData) {
		if (storedPayloadData.has_value()) { // Should always be set.
			CATAPULT_LOG(debug) << "[DBRB] DELIVER: delivering payload";
			m_deliverCallback(storedPayloadData->Payload);
		} else {
			CATAPULT_LOG(error) << "[DBRB] DELIVER: NO PAYLOAD!!!";
		}

		onLeaveAllowed();
	}


	// Other callbacks:

	void DbrbProcess::onViewDiscovered(View&& newView) {
		if (!m_viewDiscoveryActive)
			return;

		m_currentView = std::move(newView);

		if (m_membershipState == MembershipState::Joining && m_disseminateReconfig) {
			auto pMessage = std::make_shared<ReconfigMessage>(m_id, m_id, MembershipChange::Join, m_currentView);
			disseminate(pMessage, m_currentView.members());
		}

		// If the process is leaving after an Install message.
		if (m_disseminateCommit) {
			// All fields in m_storedPayloadData are guaranteed to exist.
//			auto pMessage = std::make_shared<CommitMessage>(m_id, m_storedPayloadData->Payload, m_storedPayloadData->Certificate, m_storedPayloadData->CertificateView, m_currentView);
//			disseminate(pMessage, m_currentView.members());
		}
	}

	void DbrbProcess::onViewInstalled(const View& newView) {
		CATAPULT_LOG(debug) << "[DBRB] VIEW INSTALLED: New view is being installed.";
		// Resume processing Prepare, Commit and Reconfig messages.
		m_limitedProcessing = false;

		if (!m_pendingChanges.Data.empty() && !m_proposedSequences.count(newView)) {
			CATAPULT_LOG(debug) << "[DBRB] VIEW INSTALLED: Pending changes exist AND new view exists in proposed sequences.";
			const auto mergedView = View::merge(newView, m_pendingChanges);
			const auto sequence = Sequence::fromViews({ mergedView });	// Single view is always a valid sequence.
			m_proposedSequences[newView] = *sequence;

			auto pMessage = std::make_shared<ProposeMessage>(m_id, *sequence, newView);
			disseminate(pMessage, newView.members());
		}

//		const auto& pAcknowledgeable = m_state.Acknowledgeable;
//		const bool processIsSender = pAcknowledgeable.has_value() && pAcknowledgeable->Sender == m_id;
//		const bool certificateExists = !m_certificate.empty();
//
//		if (processIsSender && !certificateExists) {
//			CATAPULT_LOG(debug) << "[DBRB] VIEW INSTALLED: Node is a payload sender, but certificate does not exist.";
//			auto pMessage = std::make_shared<PrepareMessage>(m_id, pAcknowledgeable->Payload, newView);
//			disseminate(pMessage, newView.members());
//		}
//
//		if (processIsSender && certificateExists && !m_canLeave) {
//			CATAPULT_LOG(debug) << "[DBRB] VIEW INSTALLED: Node is a payload sender with a valid certificate AND leave is not allowed.";
//			// If m_certificate exists, then m_certificateView also exists.
//			// TODO: Double-check which payload should be used.
//			auto pMessage = std::make_shared<CommitMessage>(m_id, m_storedPayloadData->Payload, m_certificate, m_certificateView, newView);
//			disseminate(pMessage, newView.members());
//		}
//
//		if (!processIsSender && m_storedPayloadData.has_value() && !m_canLeave) {
//			CATAPULT_LOG(debug) << "[DBRB] VIEW INSTALLED: Node is not a payload sender AND stored payload exists AND leave is not allowed.";
//			auto pMessage = std::make_shared<CommitMessage>(m_id, m_storedPayloadData->Payload, m_storedPayloadData->Certificate, m_storedPayloadData->CertificateView, newView);
//			disseminate(pMessage, newView.members());
//		}

		// Disseminate Reconfig message using the installed view, if the process is leaving.
		if (m_membershipState == MembershipState::Leaving) {
			CATAPULT_LOG(debug) << "[DBRB] VIEW INSTALLED: Node is leaving.";
			auto pMessage = std::make_shared<ReconfigMessage>(m_id, m_id, MembershipChange::Leave, newView);
			disseminate(pMessage, newView.members());
		}
	}

	void DbrbProcess::onLeaveAllowed() {
		CATAPULT_LOG(debug) << "[DBRB] LEAVE: Leave is now allowed.";
		m_canLeave = true;
		if (m_disseminateCommit)
			onLeaveComplete();
	}

	void DbrbProcess::onJoinComplete() {
		CATAPULT_LOG(debug) << "[DBRB] JOIN: Completed, node is now Participating.";
		m_disseminateReconfig = false;
//		m_viewDiscoveryActive = false;	// TODO: Not clear if view discovery should remain active or not
		m_membershipState = MembershipState::Participating;
	}

	void DbrbProcess::onLeaveComplete() {
		CATAPULT_LOG(debug) << "[DBRB] LEAVE: Completed, node is now Left.";
		m_disseminateReconfig = false;
		m_disseminateCommit = false;
		m_viewDiscoveryActive = false;
		m_membershipState = MembershipState::Left;
	}
}}