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
#include "catapult/utils/NetworkTime.h"
#include <utility>


namespace catapult { namespace dbrb {

	DbrbProcess::DbrbProcess(
		const std::weak_ptr<net::PacketWriters>& pWriters,
		const net::PacketIoPickerContainer& packetIoPickers,
		const ionet::Node& thisNode,
		const crypto::KeyPair& keyPair,
		std::shared_ptr<thread::IoThreadPool> pPool,
		std::shared_ptr<TransactionSender> pTransactionSender,
		const dbrb::DbrbViewFetcher& dbrbViewFetcher)
			: m_id(thisNode.identityKey())
			, m_keyPair(keyPair)
			, m_nodeRetreiver(packetIoPickers, thisNode.metadata().NetworkIdentifier, m_id, pWriters)
			, m_pMessageSender(std::make_shared<MessageSender>(pWriters, m_nodeRetreiver))
			, m_pPool(std::move(pPool))
			, m_strand(m_pPool->ioContext())
			, m_pTransactionSender(std::move(pTransactionSender))
			, m_dbrbViewFetcher(dbrbViewFetcher) {
		m_node.Node = thisNode;
		auto pPackedNode = ionet::PackNode(thisNode);
		// Skip NetworkNode fields: size, host and friendly name.
		auto hash = CalculateHash({ { reinterpret_cast<const uint8_t*>(pPackedNode.get()) + sizeof(uint32_t), sizeof(ionet::NetworkNode) - sizeof(uint32_t) - 2 * sizeof(uint8_t) } });
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

	void DbrbProcess::setValidationCallback(const ValidationCallback& callback) {
		m_validationCallback = callback;
	}

	void DbrbProcess::setDeliverCallback(const DeliverCallback& callback) {
		m_deliverCallback = callback;
	}

	NodeRetreiver& DbrbProcess::nodeRetreiver() {
		return m_nodeRetreiver;
	}

	boost::asio::io_context::strand& DbrbProcess::strand() {
		return m_strand;
	}

	MessageSender& DbrbProcess::messageSender() {
		return *m_pMessageSender;
	}

	const View& DbrbProcess::currentView() {
		return m_currentView;
	}

	// Basic operations:

	void DbrbProcess::broadcast(const Payload& payload) {
		CATAPULT_LOG(debug) << "[DBRB] BROADCAST: payload " << payload->Type;
		boost::asio::post(m_strand, [pThisWeak = weak_from_this(), payload]() {
			CATAPULT_LOG(trace) << "[DBRB] BROADCAST: stranded broadcast call for payload " << payload->Type;
			auto pThis = pThisWeak.lock();
			if (!pThis)
				return;

			if (!pThis->m_currentView.isMember(pThis->m_id)) {
				CATAPULT_LOG(debug) << "[DBRB] BROADCAST: not a member of the current view " << pThis->m_currentView << ", aborting broadcast.";
				return;
			}

			CATAPULT_LOG(trace) << "[DBRB] BROADCAST: sending payload " << payload->Type;
			auto pMessage = std::make_shared<PrepareMessage>(pThis->m_id, payload, pThis->m_currentView);
			pThis->disseminate(pMessage, pMessage->View.Data);
		});
	}

	void DbrbProcess::processMessage(const Message& message) {
		switch (message.Type) {
			case ionet::PacketType::Dbrb_Prepare_Message: {
				CATAPULT_LOG(trace) << "[DBRB] Received PREPARE message from " << message.Sender << ".";
				onPrepareMessageReceived(dynamic_cast<const PrepareMessage&>(message));
				break;
			}
			case ionet::PacketType::Dbrb_Acknowledged_Message: {
				CATAPULT_LOG(trace) << "[DBRB] Received ACKNOWLEDGED message from " << message.Sender << ".";
				onAcknowledgedMessageReceived(dynamic_cast<const AcknowledgedMessage&>(message));
				break;
			}
			case ionet::PacketType::Dbrb_Commit_Message: {
				CATAPULT_LOG(trace) << "[DBRB] Received COMMIT message from " << message.Sender << ".";
				onCommitMessageReceived(dynamic_cast<const CommitMessage&>(message));
				break;
			}
			case ionet::PacketType::Dbrb_Deliver_Message: {
				CATAPULT_LOG(trace) << "[DBRB] Received DELIVER message from " << message.Sender << ".";
				onDeliverMessageReceived(dynamic_cast<const DeliverMessage&>(message));
				break;
			}
			default:
				CATAPULT_THROW_RUNTIME_ERROR_1("invalid DBRB message type", message.Type)
		}
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
		// Forms a hash based on payload and current view and signs it.

		uint32_t packetPayloadSize = m_currentView.packedSize();
		auto pPacket = ionet::CreateSharedPacket<ionet::Packet>(packetPayloadSize);
		auto pBuffer = pPacket->Data();
		Write(pBuffer, m_currentView);

		auto hash = CalculateHash({ { reinterpret_cast<const uint8_t*>(payload.get()), payload->Size }, { pPacket->Data(), packetPayloadSize } });
		Signature signature;
		crypto::Sign(m_keyPair, hash, signature);

		return signature;
	}

	bool DbrbProcess::verify(const ProcessId& signer, const Payload& payload, const View& view, const Signature& signature) {
		// Forms a hash based on payload and current view and checks whether the signature is valid.

		uint32_t packetPayloadSize = view.packedSize();
		auto pPacket = ionet::CreateSharedPacket<ionet::Packet>(packetPayloadSize);
		auto pBuffer = pPacket->Data();
		Write(pBuffer, view);

		auto hash = CalculateHash({ { reinterpret_cast<const uint8_t*>(payload.get()), payload->Size }, { pPacket->Data(), packetPayloadSize } });

		bool res = crypto::Verify(signer, hash, signature);
		return res;
	}


	// Message callbacks:

	void DbrbProcess::onPrepareMessageReceived(const PrepareMessage& message) {
		CATAPULT_LOG(trace) << "[DBRB] PREPARE: received payload " << message.Payload->Type << " from " << message.Sender;
		if (!m_validationCallback(message.Payload)) {
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: Aborting message processing (message invalid).";
			return;
		}

		if (!m_currentView.isMember(m_id)) {
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: Aborting message processing (node is not a participant).";
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
		if (data.Payload) {
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: Duplicate Prepare message from " << message.Sender << ", payload hash: " << payloadHash;
			return;
		}

		data.Begin = utils::NetworkTime();

		data.Sender = message.Sender;
		data.Payload = message.Payload;

		CATAPULT_LOG(trace) << "[DBRB] PREPARE: Sending Acknowledged message to " << message.Sender << ".";
		Signature payloadSignature = sign(message.Payload);
		auto pMessage = std::make_shared<AcknowledgedMessage>(m_id, payloadHash, m_currentView, payloadSignature);
		send(pMessage, message.Sender);
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

		CATAPULT_LOG(trace) << "[DBRB] ACKNOWLEDGED: payload " << data.Payload->Type << " from " << data.Sender;

		// Signature must be valid.
		if (!verify(message.Sender, data.Payload, message.View, message.PayloadSignature)) {
			CATAPULT_LOG(warning) << "[DBRB] ACKNOWLEDGED: message with payload " << data.Payload->Type << " from " << data.Sender << " REJECTED: signature is not valid";
			return;
		}

		data.Signatures[std::make_pair(message.View, message.Sender)] = message.PayloadSignature;
		bool quorumCollected = data.QuorumManager.update(message, data.Payload->Type);
		if (quorumCollected && data.Certificate.empty())
			onAcknowledgedQuorumCollected(message);
	}

	void DbrbProcess::onAcknowledgedQuorumCollected(const AcknowledgedMessage& message) {
		// Replacing certificate.
		auto& data = m_broadcastData[message.PayloadHash];
		CATAPULT_LOG(trace) << "[DBRB] ACKNOWLEDGED: Quorum collected in view " << message.View << ". Payload " << data.Payload->Type << " from " << data.Sender;
		data.CertificateView = message.View;
		data.Certificate.clear();
		const auto& acknowledgedSet = data.QuorumManager.AcknowledgedPayloads[message.View];
		for (const auto& [processId, hash] : acknowledgedSet) {
			auto iter = data.Signatures.find(std::make_pair(message.View, processId));
			if (hash == message.PayloadHash && data.Signatures.end() != iter)
				data.Certificate[processId] = iter->second;
		}

		// Disseminating Commit message.
		CATAPULT_LOG(trace) << "[DBRB] ACKNOWLEDGED: Disseminating Commit message with payload " << data.Payload->Type << " from " << data.Sender;
		auto pMessage = std::make_shared<CommitMessage>(m_id, message.PayloadHash, data.Certificate, data.CertificateView, m_currentView);
		data.CommitMessageReceived = true;
		disseminate(pMessage, m_currentView.Data);
	}

	void DbrbProcess::onCommitMessageReceived(const CommitMessage& message) {
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

		CATAPULT_LOG(trace) << "[DBRB] COMMIT: payload " << data.Payload->Type << " from " << data.Sender;

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

			CATAPULT_LOG(trace) << "[DBRB] COMMIT: Disseminating Commit message with payload " << data.Payload->Type << " from " << data.Sender;
			auto pMessage = std::make_shared<CommitMessage>(m_id, message.PayloadHash, message.Certificate, message.CertificateView, m_currentView);
			disseminate(pMessage, m_currentView.Data);
		}

		// Allow delivery for sender process.
		CATAPULT_LOG(trace) << "[DBRB] COMMIT: Sending Deliver message with payload " << data.Payload->Type << " from " << data.Sender << " to " << message.Sender;
		auto pMessage = std::make_shared<DeliverMessage>(m_id, message.PayloadHash, m_currentView);
		send(pMessage, message.Sender);
	}

	void DbrbProcess::onDeliverMessageReceived(const DeliverMessage& message) {
		if (!m_currentView.isMember(m_id)) {
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

		CATAPULT_LOG(trace) << "[DBRB] DELIVER: payload " << data.Payload->Type << " from " << data.Sender;

		bool quorumCollected = data.QuorumManager.update(message, data.Payload->Type);
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
	}

	// Other callbacks:

	namespace {
		void LogTime(const char* prefix, const Timestamp& timestamp) {
			auto time = std::chrono::system_clock::to_time_t(utils::ToTimePoint(timestamp));
			char buffer[40];
			std::strftime(buffer, 40 ,"%F %T", std::localtime(&time));
			CATAPULT_LOG(debug) << prefix << buffer;
		}
	}

	bool DbrbProcess::updateView(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder, const Timestamp& now, const Height& height, bool registerSelf) {
		auto view = View{ m_dbrbViewFetcher.getView(now) };
		auto isTemporaryProcess = view.isMember(m_id);

		CATAPULT_LOG(debug) << "[DBRB] getting config at height " << height;
		const auto& config = pConfigHolder->Config(height).Network;
		auto bootstrapView = View{ config.DbrbBootstrapProcesses };
		if (bootstrapView.Data.empty()) {
			CATAPULT_LOG(debug) << "[DBRB] no bootstrap nodes, getting config at height " << height + Height(1);
			bootstrapView = View{ pConfigHolder->Config(height + Height(1)).Network.DbrbBootstrapProcesses };
		}
		auto isBootstrapProcess = bootstrapView.isMember(m_id);

		view.merge(bootstrapView);
		if (view.Data.empty())
			CATAPULT_THROW_RUNTIME_ERROR("no DBRB processes")

		boost::asio::post(m_strand, [pThisWeak = weak_from_this(), pConfigHolder, now, view, isTemporaryProcess, isBootstrapProcess, gracePeriod = Timestamp(config.DbrbRegistrationGracePeriod.millis()), registerSelf]() {
			auto pThis = pThisWeak.lock();
			if (!pThis)
				return;

			pThis->m_broadcastData.clear();

			pThis->m_nodeRetreiver.requestNodes(view.Data);
			pThis->m_currentView = view;
			CATAPULT_LOG(debug) << "[DBRB] Current view is now set to " << pThis->m_currentView;

			if (registerSelf) {
				bool isRegistrationRequired = false;
				if (!isTemporaryProcess && !isBootstrapProcess) {
					CATAPULT_LOG(debug) << "[DBRB] node is not registered in the DBRB system";
					isRegistrationRequired = true;
				} else if (isTemporaryProcess) {
					auto expirationTime = pThis->m_dbrbViewFetcher.getExpirationTime(pThis->m_id);
					LogTime("[DBRB] process expires at ", expirationTime);
					if (expirationTime < gracePeriod)
						CATAPULT_THROW_RUNTIME_ERROR_1("invalid expiration time", pThis->m_id)

					auto gracePeriodStart = expirationTime - gracePeriod;
					LogTime("[DBRB] process grace period starts at ", gracePeriodStart);
					if (now >= gracePeriodStart) {
						CATAPULT_LOG(debug) << "[DBRB] node registration in the DBRB system soon expires";
						isRegistrationRequired = true;
					}
				}

				if (isRegistrationRequired)
					pThis->m_pTransactionSender->sendAddDbrbProcessTransaction();
			}

			pThis->m_nodeRetreiver.broadcastNodes();
		});

		return isTemporaryProcess || isBootstrapProcess;
	}
}}