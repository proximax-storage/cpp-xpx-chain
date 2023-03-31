/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DbrbProcess.h"
#include "catapult/dbrb/Messages.h"
#include "catapult/crypto/Signer.h"
#include "catapult/ionet/NetworkNode.h"
#include "catapult/ionet/PacketPayload.h"
#include "catapult/thread/IoThreadPool.h"
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
			, m_pMessageSender(std::make_shared<MessageSender>(std::move(pWriters), m_nodeRetreiver))
			, m_strand(pPool->ioContext())
			, m_transactionSender(std::move(transactionSender)) {
		m_node.Node = thisNode;
		auto pPackedNode = ionet::PackNode(thisNode);
		auto hash = CalculateHash({ { reinterpret_cast<const uint8_t*>(pPackedNode.get()), pPackedNode->Size } });
		crypto::Sign(m_keyPair, hash, m_node.Signature);
		m_nodeRetreiver.addNodes({ m_node });
	}

	void DbrbProcess::registerPacketHandlers(ionet::ServerPacketHandlers& packetHandlers) {
		auto handler = [pThisWeak = weak_from_this(), &converter = m_converter, &strand = m_strand](const auto& packet, auto& context) {
			auto pThis = pThisWeak.lock();
			if (!pThis)
				return;

			const auto& messagePacket = static_cast<const MessagePacket&>(packet);
			auto hash = CalculateHash(messagePacket.buffers());
			if (!crypto::Verify(messagePacket.Sender, hash, messagePacket.Signature))
				return;

			auto pMessage = converter.toMessage(packet);
			boost::asio::post(strand, [pThisWeak, pMessage]() {
				auto pThis = pThisWeak.lock();
				if (pThis)
					pThis->processMessage(*pMessage);
			});
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

	NodeRetreiver& DbrbProcess::nodeRetreiver() {
		return m_nodeRetreiver;
	}

	// Basic operations:

	void DbrbProcess::leave() {
		boost::asio::post(m_strand, [pThisWeak = weak_from_this()]() {
			auto pThis = pThisWeak.lock();
			if (!pThis)
				return;

			// Can only request to leave when the process is participating in the system.
			if (pThis->m_membershipState != MembershipState::Participating)
				return;

//			const auto& pAcknowledgeable = m_state.Acknowledgeable;
//			const bool processIsSender = pAcknowledgeable.has_value() && pAcknowledgeable->Sender == m_id;
//			bool needsPermissionToLeave = m_payloadIsDelivered || processIsSender;
//			if (needsPermissionToLeave && !m_canLeave)
//				return;	// TODO: Can notify user about the reason why leave failed

			pThis->m_membershipState = MembershipState::Leaving;

			// Disseminate a Reconfig message, if current view is installed...
			if (pThis->m_currentViewIsInstalled) {
				const auto& currentView = pThis->m_currentView;
				auto pMessage = std::make_shared<ReconfigMessage>(pThis->m_id, pThis->m_id, MembershipChange::Leave, currentView);
				pThis->disseminate(pMessage, currentView.members());
			}

			// ...and keep disseminating new Reconfig messages whenever a new view is installed.
			pThis->m_disseminateReconfig = true;
		});
	}

	void DbrbProcess::broadcast(const Payload& payload) {
		CATAPULT_LOG(debug) << "[DBRB] BROADCAST: payload " << payload->Type;
		boost::asio::post(m_strand, [pThisWeak = weak_from_this(), payload]() {
			CATAPULT_LOG(debug) << "[DBRB] BROADCAST: stranded broadcast call for payload " << payload->Type;
			auto pThis = pThisWeak.lock();
			if (!pThis)
				return;

			// TODO: fix view search in set.
			if (!pThis->m_currentViewIsInstalled) {
				CATAPULT_LOG(debug) << "[DBRB] BROADCAST: Current view is not installed, aborting broadcast.";
				return; // The process waits to install some view and then disseminates the prepare message.
				// TODO: Can notify user about the reason why broadcast failed
			}

			if (!pThis->m_currentView.isMember(pThis->m_id)) {
				CATAPULT_LOG(debug) << "[DBRB] BROADCAST: not a member of the current view " << pThis->m_currentView << ", aborting broadcast.";
				return;
			}

			CATAPULT_LOG(debug) << "[DBRB] BROADCAST: sending payload " << payload->Type;
			auto pMessage = std::make_shared<PrepareMessage>(pThis->m_id, payload, pThis->m_currentView);
			pThis->disseminate(pMessage, pMessage->View.members());
		});
	}

	void DbrbProcess::processMessage(const Message& message) {
		switch (message.Type) {
			case ionet::PacketType::Dbrb_Reconfig_Message: {
				CATAPULT_LOG(error) << "[DBRB] Received RECONFIG message from " << message.Sender << ".";
				onReconfigMessageReceived(dynamic_cast<const ReconfigMessage&>(message));
				break;
			}
			case ionet::PacketType::Dbrb_Reconfig_Confirm_Message: {
				CATAPULT_LOG(error) << "[DBRB] Received RECONFIG-CONFIRM message from " << message.Sender << ".";
				onReconfigConfirmMessageReceived(dynamic_cast<const ReconfigConfirmMessage&>(message));
				break;
			}
			case ionet::PacketType::Dbrb_Propose_Message: {
				CATAPULT_LOG(error) << "[DBRB] Received PROPOSE message from " << message.Sender << ".";
				onProposeMessageReceived(dynamic_cast<const ProposeMessage&>(message));
				break;
			}
			case ionet::PacketType::Dbrb_Converged_Message: {
				CATAPULT_LOG(error) << "[DBRB] Received CONVERGED message from " << message.Sender << ".";
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
		boost::asio::post(m_strand, [pThisWeak = weak_from_this()]() {
			auto pThis = pThisWeak.lock();
			if (pThis)
				pThis->m_broadcastData.clear();
		});
	}

	// Basic private methods:

	void DbrbProcess::disseminate(const std::shared_ptr<Message>& pMessage, std::set<ProcessId> recipients) {
		auto pPacket = pMessage->toNetworkPacket(&m_keyPair);
		for (auto iter = recipients.begin(); iter != recipients.end(); ++iter) {
			if (m_id == *iter) {
				boost::asio::post(m_strand, [pThisWeak = weak_from_this(), pMessage]() {
					auto pThis = pThisWeak.lock();
					if (pThis)
						pThis->processMessage(*pMessage);
				});
				recipients.erase(iter);
				break;
			}
		}

		m_pMessageSender->send(pPacket, recipients);
	}

	void DbrbProcess::send(const std::shared_ptr<Message>& pMessage, const ProcessId& recipient) {
		disseminate(pMessage, std::set<ProcessId>{ recipient });
	}

	void DbrbProcess::prepareForStateUpdates(const InstallMessageData& installMessageData) {
		m_currentInstallMessage = installMessageData;
		if (m_quorumManager.StateUpdateMessages.find(installMessageData.ReplacedView) == m_quorumManager.StateUpdateMessages.end())
			m_quorumManager.StateUpdateMessages[installMessageData.ReplacedView] = {};
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
				auto iter = m_broadcastData.find(pStored->PayloadHash);
				if(iter != m_broadcastData.end()) {
					const auto& certificate = pStored->Certificate;
					const auto& certificateView = pStored->CertificateView;
					const auto& payload = iter->second.Payload;
					if (payload) {
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
		CATAPULT_LOG(debug) << "[DBRB] RECONFIG: Current pending changes: " << m_pendingChanges;

		if (!m_currentViewIsInstalled)
			return;

		auto newView = m_currentView;
		newView.merge(m_pendingChanges);
		const std::vector<View> sequenceData{ newView };

		m_proposedSequences[m_currentView] = Sequence::fromViews(sequenceData).value();
		const auto& proposedSequence = m_proposedSequences.at(m_currentView);
		CATAPULT_LOG(debug) << "[DBRB] RECONFIG: Proposed sequence to replace " << m_currentView << " is SET to " << proposedSequence;

		CATAPULT_LOG(debug) << "[DBRB] RECONFIG: Disseminating Propose message in the view " << m_currentView;
		auto pMessage = std::make_shared<ProposeMessage>(m_id, proposedSequence, m_currentView);
		disseminate(pMessage, m_currentView.members());
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

		auto pMessage = std::make_shared<ReconfigConfirmMessage>(m_id, message.View);
		send(pMessage, message.Sender);

		View appendedView;
		appendedView.Data.emplace(message.ProcessId, message.MembershipChange);
		extendPendingChanges(appendedView);
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
	}

	void DbrbProcess::onProposeMessageReceived(const ProposeMessage& message) {
		// Must be sent from a member of replaced view.
		if (!message.ReplacedView.isMember(message.Sender)) {
			CATAPULT_LOG(debug) << "[DBRB] PROPOSE: sender is not a member of replaced view.";
			return;
		}

		// Every view in ProposedSequence must be more recent than the current view of the process.
		const auto pLeastRecentView = message.ProposedSequence.maybeLeastRecent();
		if ( !pLeastRecentView || m_currentView >= *pLeastRecentView ) {
			CATAPULT_LOG(debug) << "[DBRB] PROPOSE: proposed sequence is not well-formed.";
			return;
		}

		// TODO: Check that there is at least one view in ProposedSequence that the process is not aware of

		auto& currentSequence = m_proposedSequences[message.ReplacedView];
		CATAPULT_LOG(debug) << "[DBRB] PROPOSE: Current sequence for view " << message.ReplacedView << " is " << currentSequence;
		CATAPULT_LOG(debug) << "[DBRB] PROPOSE: Proposed sequence is " << message.ProposedSequence;

		bool conflicting = !currentSequence.canMerge(message.ProposedSequence);
		if (conflicting) {
			CATAPULT_LOG(debug) << "[DBRB] PROPOSE: Sequences are conflicting.";
			const auto localMostRecent = currentSequence.maybeMostRecent().value_or(View{});
			const auto proposedMostRecent = message.ProposedSequence.maybeMostRecent().value_or(View{});
			const auto mergedView = View::merge(localMostRecent, proposedMostRecent);

			auto lastConverged = m_lastConvergedSequences[message.ReplacedView];
			CATAPULT_LOG(debug) << "[DBRB] PROPOSE: Merging LCSEQ " << lastConverged << " + {" << localMostRecent << " + " << proposedMostRecent << "}";
			lastConverged.tryAppend(mergedView);

			currentSequence = lastConverged;
		} else {
			currentSequence.tryMerge(message.ProposedSequence);	// Will always succeed.
		}
		CATAPULT_LOG(debug) << "[DBRB] PROPOSE: Current sequence for view " << message.ReplacedView << " is " << currentSequence;

		// Updating quorum counter for received Propose message.
		bool quorumCollected = m_quorumManager.update(message);
		if (quorumCollected)
			onProposeQuorumCollected(message);

		// If the message was received from self, don't disseminate it to other nodes.
		if (message.Sender == m_id)
			return;

		CATAPULT_LOG(debug) << "[DBRB] PROPOSE: Disseminating Propose message.";
		auto pMessage = std::make_shared<ProposeMessage>(m_id, currentSequence, message.ReplacedView);
		auto recipients = message.ReplacedView.members();
		auto quorumIds = m_quorumManager.ProposedCounters[std::make_pair(message.ReplacedView, message.ProposedSequence)];
		for (const auto& id : quorumIds)
			recipients.erase(id);
		disseminate(pMessage, recipients);
	}

	void DbrbProcess::onProposeQuorumCollected(const ProposeMessage& message) {
		CATAPULT_LOG(debug) << "[DBRB] PROPOSE: Quorum collected in view " << message.ReplacedView << ".";
		m_lastConvergedSequences[message.ReplacedView] = message.ProposedSequence;

		CATAPULT_LOG(debug) << "[DBRB] PROPOSE: Disseminating Converged message.";
		auto pMessage = std::make_shared<ConvergedMessage>(m_id, message.ProposedSequence, message.ReplacedView);
		disseminate(pMessage, message.ReplacedView.members());
	}

	void DbrbProcess::onConvergedMessageReceived(const ConvergedMessage& message) {
		CATAPULT_LOG(error) << "[DBRB] CONVERGED: Received message from " << message.Sender;
		CATAPULT_LOG(debug) << "[DBRB] CONVERGED: Signature = " << message.Signature;
		CATAPULT_LOG(debug) << "[DBRB] CONVERGED: Replaced view = " << message.ReplacedView;
		CATAPULT_LOG(debug) << "[DBRB] CONVERGED: Converged sequence = " << message.ConvergedSequence;
		bool quorumCollected = m_quorumManager.update(message);
		if (quorumCollected)
			onConvergedQuorumCollected(message);
	}

	void DbrbProcess::onConvergedQuorumCollected(const ConvergedMessage& message) {
		CATAPULT_LOG(debug) << "[DBRB] CONVERGED: Quorum collected in view " << message.ReplacedView << ".";
		Sequence sequence;
		sequence.tryAppend(message.ReplacedView);
		sequence.tryAppend(message.ConvergedSequence);

		const auto& convergedSignatures = m_quorumManager.ConvergedSignatures.at(sequence);

		auto pMessage = std::make_shared<InstallMessage>(m_id, sequence, convergedSignatures);

		const auto& mostRecentView = message.ConvergedSequence.maybeMostRecent().value_or(View{});
		std::set<ProcessId> replacedViewMembers = message.ReplacedView.members();
		std::set<ProcessId> mostRecentViewMembers = mostRecentView.members();
		std::set<ProcessId> recipientsUnion;
		std::set_union(replacedViewMembers.begin(), replacedViewMembers.end(),
			mostRecentViewMembers.begin(), mostRecentViewMembers.end(),
			std::inserter(recipientsUnion, recipientsUnion.begin()));

		View recipientsView;
		for (const auto& processId : recipientsUnion)
			recipientsView.Data.emplace(processId, MembershipChange::Join);
		CATAPULT_LOG(error) << "[DBRB] CONVERGED: Disseminating Install message to " << recipientsView;
		disseminate(pMessage, recipientsUnion);
	}

	void DbrbProcess::onInstallMessageReceived(const InstallMessage& message) {
		const auto pInstallMessageData = message.tryGetMessageData();
		if (!pInstallMessageData.has_value()) {
			CATAPULT_LOG(warning) << "[DBRB] INSTALL: message is ill-formed.";
			return;
		}

		const auto& replacedView = pInstallMessageData->ReplacedView;
		const auto& convergedSequence = pInstallMessageData->ConvergedSequence;
		const auto& mostRecentView = pInstallMessageData->MostRecentView;

		if (replacedView.isMember(m_id)) {
			if (m_currentView < mostRecentView)
				m_limitedProcessing = true;	// Stop processing Prepare, Commit and Reconfig messages.

			// TODO: Double-check; not clear what was meant by state(v)
			auto pMessage = std::make_shared<StateUpdateMessage>(m_id, m_state, replacedView, m_pendingChanges);

			std::set<ProcessId> replacedViewMembers = replacedView.members();
			std::set<ProcessId> mostRecentViewMembers = mostRecentView.members();
			std::set<ProcessId> recipientsUnion;
			std::set_union(replacedViewMembers.begin(), replacedViewMembers.end(),
				mostRecentViewMembers.begin(), mostRecentViewMembers.end(),
				std::inserter(recipientsUnion, recipientsUnion.begin()));

			disseminate(pMessage, recipientsUnion);
		}

		if (m_currentView < mostRecentView) {
			// Wait until a quorum of StateUpdate messages is collected for (message.ReplacedView).
			CATAPULT_LOG(debug) << "[DBRB] INSTALL: Preparing for state updates.";
			prepareForStateUpdates(*pInstallMessageData);
		}
	}

	void DbrbProcess::onPrepareMessageReceived(const PrepareMessage& message) {
		if (m_membershipState != MembershipState::Participating) {
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: Aborting message processing (node is not a participant).";
			return;
		}

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

		auto payloadHash = CalculatePayloadHash(message.Payload);
		auto& data = m_broadcastData[payloadHash];
		if (data.Payload)
			CATAPULT_THROW_RUNTIME_ERROR_2("duplicate prepare message", data.Payload, message.Sender)

		data.Begin = utils::NetworkTime();

		data.Sender = message.Sender;
		CATAPULT_LOG(debug) << "[DBRB] PREPARE: No acknowledgeable payload is stored.";
		data.Payload = message.Payload;
		if (!m_state.Acknowledgeable.has_value()) {
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: Setting acknowledgeable payload to one from the message.";
			m_state.Acknowledgeable = message;
		}

		CATAPULT_LOG(debug) << "[DBRB] PREPARE: Sending Acknowledged message to " << message.Sender << ".";
		Signature payloadSignature = sign(message.Payload);
		auto pMessage = std::make_shared<AcknowledgedMessage>(m_id, payloadHash, m_currentView, payloadSignature);
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
		const auto& convergedSequence = m_currentInstallMessage->ConvergedSequence;
		const auto& mostRecentView = m_currentInstallMessage->MostRecentView;

		// Updating pending changes.
		View reconfigRequests;
		for (const auto& message : stateUpdateMessages) {
			reconfigRequests.merge(message.PendingChanges);
		}
		reconfigRequests.difference(mostRecentView);
		extendPendingChanges(reconfigRequests);

		// Uninstalling current view.
		m_currentViewIsInstalled = false;

		// Updating state and payload-related fields of the process.
		updateState(stateUpdateMessages);

		CATAPULT_LOG(debug) << "[DBRB] STATE UPDATE: Most recent view is " << mostRecentView;
		if (mostRecentView.isMember(m_id)) {
			m_currentView = mostRecentView;
			CATAPULT_LOG(debug) << "[DBRB] STATE UPDATE: Node is in the least recent view. Updated current view to " << mostRecentView;

			if (!m_currentInstallMessage->ReplacedView.isMember(m_id))
				onJoinComplete();

			// Check if there are more recent views in the sequence of the install message.
			Sequence moreRecentSequence;
			for (auto iter = convergedSequence.data().rbegin(); iter != convergedSequence.data().rend() && m_currentView < *iter; ++iter)
				moreRecentSequence.tryInsert(*iter);

			if (!moreRecentSequence.data().empty()
					&& m_proposedSequences.count(m_currentView) == 0
					&& m_currentView < moreRecentSequence.maybeLeastRecent().value_or(View{})) {
				m_proposedSequences[m_currentView] = moreRecentSequence;

				auto pMessage = std::make_shared<ProposeMessage>(m_id, moreRecentSequence, m_currentView);
				disseminate(pMessage, m_currentView.members());
			} else {
				m_currentViewIsInstalled = true;

				Sequence sequence;
				sequence.tryAppend(m_currentInstallMessage->ReplacedView);
				sequence.tryAppend(m_currentInstallMessage->ConvergedSequence);
				InstallMessage installMessage(ProcessId(), std::move(sequence), m_currentInstallMessage->ConvergedSignatures);
				m_transactionSender.sendInstallMessageTransaction(installMessage);
				CATAPULT_LOG(debug) << "[DBRB] STATE UPDATE: sent install message transaction";

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
			CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: Aborting message processing (sender is not in supplied view).";
			return;
		}

		auto& data = m_broadcastData[message.PayloadHash];
		if (!data.Payload) {
			CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: Aborting message processing (no payload).";
			return;
		}

		CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: payload " << data.Payload->Type << " from " << data.Sender;

		// Signature must be valid.
		if (!verify(message.Sender, data.Payload, message.View, message.PayloadSignature)) {
			CATAPULT_LOG(warning) << "[DBRB] ACKNOWLEDGED: message with payload " << data.Payload->Type << " from " << data.Sender << " REJECTED: signature is not valid";
			return;
		}

		data.Signatures[std::make_pair(message.View, message.Sender)] = message.PayloadSignature;
		bool quorumCollected = data.QuorumManager.update(message);
		if (quorumCollected && data.Certificate.empty())
			onAcknowledgedQuorumCollected(message);
	}

	void DbrbProcess::onAcknowledgedQuorumCollected(const AcknowledgedMessage& message) {

		// Replacing certificate.
		auto& data = m_broadcastData[message.PayloadHash];
		CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: Quorum collected in view " << message.View << ". Payload " << data.Payload->Type << " from " << data.Sender;
		data.CertificateView = message.View;
		data.Certificate.clear();
		const auto& acknowledgedSet = data.QuorumManager.AcknowledgedPayloads[message.View];
		for (const auto& [processId, hash] : acknowledgedSet) {
			if (hash == message.PayloadHash)
				data.Certificate[processId] = data.Signatures.at(std::make_pair(message.View, processId));
		}

		// Disseminating Commit message, if process' current view is installed.
		if (m_currentViewIsInstalled) {
			CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: Disseminating Commit message with payload " << data.Payload->Type << " from " << data.Sender;
			auto pMessage = std::make_shared<CommitMessage>(m_id, message.PayloadHash, data.Certificate, data.CertificateView, m_currentView);
			disseminate(pMessage, m_currentView.members());
		} else {
			CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: Current view is not installed, Commit message with payload " << data.Payload->Type << " from " << data.Sender << " is not disseminated.";
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

		auto& data = m_broadcastData[message.PayloadHash];
		if (!data.Payload) {
			CATAPULT_LOG(debug) << "[DBRB] COMMIT: Aborting message processing (no payload).";
			return;
		}

		CATAPULT_LOG(debug) << "[DBRB] COMMIT: payload " << data.Payload->Type << " from " << data.Sender;

		// Message certificate must be valid, i.e. all signatures in it must be valid.
		for (const auto& [signer, signature] : message.Certificate) {
			if (!verify(signer, data.Payload, message.CertificateView, signature)) {
				CATAPULT_LOG(warning) << "[DBRB] COMMIT: message with payload " << data.Payload->Type << " from " << data.Sender << " is REJECTED: signature is not valid";
				return;
			}
		}

		// Update stored PayloadData and ProcessState, if necessary,
		// and disseminate Commit message with updated view.
		if (!data.CommitMessageReceived) {
			data.CommitMessageReceived = true;
			m_state.Stored = message;

			CATAPULT_LOG(debug) << "[DBRB] COMMIT: Disseminating Commit message with payload " << data.Payload->Type << " from " << data.Sender;
			auto pMessage = std::make_shared<CommitMessage>(m_id, message.PayloadHash, message.Certificate, message.CertificateView, m_currentView);
			disseminate(pMessage, m_currentView.members());
		}

		// Allow delivery for sender process.
		CATAPULT_LOG(debug) << "[DBRB] COMMIT: Sending Deliver message with payload " << data.Payload->Type << " from " << data.Sender << " to " << message.Sender;
		auto pMessage = std::make_shared<DeliverMessage>(m_id, message.PayloadHash, m_currentView);
		send(pMessage, message.Sender);
	}

	void DbrbProcess::onDeliverMessageReceived(const DeliverMessage& message) {
		if (m_membershipState != MembershipState::Participating) {
			CATAPULT_LOG(debug) << "[DBRB] DELIVER: Aborting message processing (node is not a participant).";
			return;
		}

		// Message sender must be a member of the view specified in the message.
		if (!message.View.isMember(message.Sender)) {
			CATAPULT_LOG(debug) << "[DBRB] DELIVER: Aborting message processing (sender is not in supplied view).";
			return;
		}

		auto& data = m_broadcastData[message.PayloadHash];
		if (!data.Payload) {
			CATAPULT_LOG(debug) << "[DBRB] DELIVER: Aborting message processing (no payload).";
			return;
		}

		CATAPULT_LOG(debug) << "[DBRB] DELIVER: payload " << data.Payload->Type << " from " << data.Sender;

		bool quorumCollected = data.QuorumManager.update(message);
		if (quorumCollected) {
			onDeliverQuorumCollected(data.Payload, data.Sender);

			CATAPULT_LOG(debug) << "[DBRB] BROADCAST: operation took " << (utils::NetworkTime().unwrap() - data.Begin.unwrap()) << " ms to deliver " << data.Payload->Type << " from " << data.Sender;
		}
	}

	void DbrbProcess::onDeliverQuorumCollected(const Payload& payload, const ProcessId& sender) {
		if (payload) { // Should always be set.
			CATAPULT_LOG(debug) << "[DBRB] DELIVER: delivering payload " << payload->Type << " from " << sender;
			m_deliverCallback(payload);
		} else {
			CATAPULT_LOG(error) << "[DBRB] DELIVER: NO PAYLOAD!!!";
		}

		onLeaveAllowed();
	}

	// Other callbacks:

	void DbrbProcess::onViewDiscovered(const ViewData& viewData) {
		if (viewData.empty()) {
			CATAPULT_LOG(debug) << "[DBRB] discovered view is empty";
			return;
		}

		boost::asio::post(m_strand, [pThisWeak = weak_from_this(), viewData]() {
			auto pThis = pThisWeak.lock();
			if (!pThis)
				return;


			std::set<ProcessId> ids;
			for (const auto& [id, _] : viewData)
				ids.emplace(id);
			pThis->m_nodeRetreiver.requestNodes(ids);

			pThis->m_currentView = View{ viewData };
			CATAPULT_LOG(debug) << "[DBRB] Current view is now set to " << pThis->m_currentView;
			if (pThis->m_currentView.isMember(pThis->m_id)) {
				pThis->m_membershipState = MembershipState::Participating;
				CATAPULT_LOG(debug) << "[DBRB] Current view is now installed";
				pThis->m_currentViewIsInstalled = true;
			} else if (pThis->m_currentView.hasChange(pThis->m_id, MembershipChange::Leave)) {
				pThis->m_membershipState = MembershipState::Left;
			}

			if (pThis->m_membershipState == MembershipState::NotJoined) {
				pThis->m_membershipState = MembershipState::Joining;
				pThis->m_disseminateReconfig = true;
			}

			if (pThis->m_disseminateReconfig) {
				if (pThis->m_membershipState == MembershipState::Joining) {
					auto pMessage = std::make_shared<ReconfigMessage>(pThis->m_id, pThis->m_id, MembershipChange::Join, pThis->m_currentView);
					pThis->disseminate(pMessage, pThis->m_currentView.members());
				} else if (pThis->m_membershipState == MembershipState::Leaving) {
					if (pThis->m_currentViewIsInstalled) {
						const auto& currentView = pThis->m_currentView;
						auto pMessage = std::make_shared<ReconfigMessage>(pThis->m_id, pThis->m_id, MembershipChange::Leave, currentView);
						pThis->disseminate(pMessage, currentView.members());
					}
				}
			}

			// If the process is leaving after an Install message.
			if (pThis->m_disseminateCommit) {
				// All fields in m_storedPayloadData are guaranteed to exist.
//				auto pMessage = std::make_shared<CommitMessage>(m_id, m_storedPayloadData->Payload, m_storedPayloadData->Certificate, m_storedPayloadData->CertificateView, m_currentView);
//				disseminate(pMessage, m_currentView.members());
			}
		});
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
		m_membershipState = MembershipState::Participating;
		m_quorumManager.ReconfigConfirmCounters.clear();
		m_pendingChanges.Data.clear();
	}

	void DbrbProcess::onLeaveComplete() {
		CATAPULT_LOG(debug) << "[DBRB] LEAVE: Completed, node is now Left.";
		m_disseminateReconfig = false;
		m_disseminateCommit = false;
		m_membershipState = MembershipState::Left;
		m_quorumManager.ReconfigConfirmCounters.clear();
		m_pendingChanges.Data.clear();
	}
}}