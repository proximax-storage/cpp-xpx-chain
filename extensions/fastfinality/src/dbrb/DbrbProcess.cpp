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
		packetHandlers.registerHandler(ionet::PacketType::Dbrb_Prepare_Message, handler);
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
		});
	}

	void DbrbProcess::broadcast(const Payload& payload) {
		boost::asio::post(m_strand, [pThisWeak = weak_from_this(), payload]() {
			auto pThis = pThisWeak.lock();
			if (!pThis)
				return;

			if (!pThis->m_currentView.isMember(pThis->m_id)) {
				CATAPULT_LOG(debug) << "[DBRB] BROADCAST: not a member of the current view " << pThis->m_currentView << ", aborting broadcast.";
				return;
			}

			auto pMessage = std::make_shared<PrepareMessage>(pThis->m_id, payload, pThis->m_currentView);
			pThis->disseminate(pMessage, pMessage->View.members());
		});
	}

	void DbrbProcess::processMessage(const Message& message) {
		switch (message.Type) {
			case ionet::PacketType::Dbrb_Prepare_Message: {
				CATAPULT_LOG(debug) << "[DBRB] Received PREPARE message from " << message.Sender << ".";
				onPrepareMessageReceived(dynamic_cast<const PrepareMessage&>(message));
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
		if (data.Begin == Timestamp())
			data.Begin = utils::NetworkTime();

		if (!data.Payload) {
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: No acknowledgeable payload is stored.";
			data.Payload = message.Payload;
			if (!m_state.Acknowledgeable.has_value()) {
				CATAPULT_LOG(debug) << "[DBRB] PREPARE: Setting acknowledgeable payload to one from the message.";
				m_state.Acknowledgeable = message;
			}
		}

		CATAPULT_LOG(debug) << "[DBRB] PREPARE: Sending Acknowledged message to " << message.Sender << ".";
		Signature payloadSignature = sign(message.Payload);
		auto pMessage = std::make_shared<AcknowledgedMessage>(m_id, payloadHash, m_currentView, payloadSignature);
		send(pMessage, message.Sender);
	}

	void DbrbProcess::onAcknowledgedMessageReceived(const AcknowledgedMessage& message) {
		// Message sender must be a member of the view specified in the message.
		if (!message.View.isMember(message.Sender)) {
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: Aborting message processing (sender is not in supplied view).";
			return;
		}

		auto& data = m_broadcastData[message.PayloadHash];
		if (!data.Payload) {
			CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: Aborting message processing (no payload).";
			return;
		}

		// Signature must be valid.
		if (!verify(message.Sender, data.Payload, message.View, message.PayloadSignature)) {
			CATAPULT_LOG(warning) << "[DBRB] ACKNOWLEDGED: message REJECTED: signature is not valid";
			return;
		}

		data.Signatures[std::make_pair(message.View, message.Sender)] = message.PayloadSignature;
		bool quorumCollected = data.QuorumManager.update(message);
		if (quorumCollected && data.Certificate.empty())
			onAcknowledgedQuorumCollected(message);
	}

	void DbrbProcess::onAcknowledgedQuorumCollected(const AcknowledgedMessage& message) {
		CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: Quorum collected in view " << message.View << ".";
		// Replacing view associated with the certificate.
		CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: Setting CertificateView to " << message.View << ".";

		// Replacing certificate.
		auto& data = m_broadcastData[message.PayloadHash];
		data.CertificateView = message.View;
		data.Certificate.clear();
		const auto& acknowledgedSet = data.QuorumManager.AcknowledgedPayloads[message.View];
		for (const auto& [processId, hash] : acknowledgedSet) {
			if (hash == message.PayloadHash)
				data.Certificate[processId] = data.Signatures.at(std::make_pair(message.View, processId));
		}

		// Disseminating Commit message.
		CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: Disseminating Commit message.";
		auto pMessage = std::make_shared<CommitMessage>(m_id, message.PayloadHash, data.Certificate, data.CertificateView, m_currentView);
		disseminate(pMessage, m_currentView.members());
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

		// Message certificate must be valid, i.e. all signatures in it must be valid.
		for (const auto& [signer, signature] : message.Certificate) {
			if (!verify(signer, data.Payload, message.CertificateView, signature)) {
				CATAPULT_LOG(warning) << "[DBRB] COMMIT: message REJECTED: signature is not valid";
				return;
			}
		}

		// Update stored PayloadData and ProcessState, if necessary,
		// and disseminate Commit message with updated view.
		if (!data.CommitMessageReceived) {
			data.CommitMessageReceived = true;
			m_state.Stored = message;

			CATAPULT_LOG(debug) << "[DBRB] COMMIT: Disseminating Commit message.";
			auto pMessage = std::make_shared<CommitMessage>(m_id, message.PayloadHash, message.Certificate, message.CertificateView, m_currentView);
			disseminate(pMessage, m_currentView.members());
		}

		// Allow delivery for sender process.
		CATAPULT_LOG(debug) << "[DBRB] COMMIT: Sending Deliver message to " << message.Sender << ".";
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

		bool quorumCollected = data.QuorumManager.update(message);
		if (quorumCollected) {
			onDeliverQuorumCollected(data.Payload);

			CATAPULT_LOG(debug) << "[DBRB] BROADCAST: operation took " << (utils::NetworkTime().unwrap() - data.Begin.unwrap()) << " ms";
		}
	}

	void DbrbProcess::onDeliverQuorumCollected(const Payload& payload) {
		if (payload) { // Should always be set.
			CATAPULT_LOG(debug) << "[DBRB] DELIVER: delivering payload";
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
			} else if (pThis->m_currentView.hasChange(pThis->m_id, MembershipChange::Leave)) {
				pThis->m_membershipState = MembershipState::Left;
			}

			if (pThis->m_membershipState == MembershipState::NotJoined) {
				pThis->m_membershipState = MembershipState::Joining;
			}

			// If the process is leaving after an Install message.
			if (pThis->m_disseminateCommit) {
				// All fields in m_storedPayloadData are guaranteed to exist.
//				auto pMessage = std::make_shared<CommitMessage>(m_id, m_storedPayloadData->Payload, m_storedPayloadData->Certificate, m_storedPayloadData->CertificateView, m_currentView);
//				disseminate(pMessage, m_currentView.members());
			}
		});
	}

	void DbrbProcess::onLeaveAllowed() {
		CATAPULT_LOG(debug) << "[DBRB] LEAVE: Leave is now allowed.";
		m_canLeave = true;
		if (m_disseminateCommit)
			onLeaveComplete();
	}

	void DbrbProcess::onJoinComplete() {
		CATAPULT_LOG(debug) << "[DBRB] JOIN: Completed, node is now Participating.";
		m_membershipState = MembershipState::Participating;
	}

	void DbrbProcess::onLeaveComplete() {
		CATAPULT_LOG(debug) << "[DBRB] LEAVE: Completed, node is now Left.";
		m_disseminateCommit = false;
		m_membershipState = MembershipState::Left;
	}
}}