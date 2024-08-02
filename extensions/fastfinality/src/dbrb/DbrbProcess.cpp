/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DbrbProcess.h"
#include "DelayedExecutor.h"
#include "catapult/dbrb/Messages.h"
#include "catapult/crypto/Signer.h"
#include "catapult/ionet/NetworkNode.h"
#include "catapult/ionet/PacketPayload.h"
#include "catapult/thread/IoThreadPool.h"
#include "catapult/utils/NetworkTime.h"
#include <utility>


namespace catapult { namespace dbrb {

	DbrbProcess::DbrbProcess(
		const crypto::KeyPair& keyPair,
		std::shared_ptr<MessageSender> pMessageSender,
		std::shared_ptr<thread::IoThreadPool> pPool,
		std::shared_ptr<TransactionSender> pTransactionSender,
		const dbrb::DbrbViewFetcher& dbrbViewFetcher)
			: m_keyPair(keyPair)
			, m_id(keyPair.publicKey())
			, m_pMessageSender(std::move(pMessageSender))
			, m_pPool(std::move(pPool))
			, m_strand(m_pPool->ioContext())
			, m_pTransactionSender(std::move(pTransactionSender))
			, m_dbrbViewFetcher(dbrbViewFetcher)
			, m_pDelayedExecutor(CreateDelayedExecutor(m_pPool))
	{}

	void DbrbProcess::registerPacketHandlers(ionet::ServerPacketHandlers& packetHandlers) {
		auto handler = [pThisWeak = weak_from_this(), &converter = m_converter, &strand = m_strand](const auto& packet, auto& context) {
			auto pMessage = converter.toMessage(packet);
			boost::asio::post(strand, [pThisWeak, pMessage]() {
				auto pThis = pThisWeak.lock();
				if (pThis)
					pThis->processMessage(*pMessage);
			});
		};
		packetHandlers.registerHandler(ionet::PacketType::Dbrb_Prepare_Message, handler);
		packetHandlers.registerHandler(ionet::PacketType::Dbrb_Acknowledged_Declined_Message, handler);
		packetHandlers.registerHandler(ionet::PacketType::Dbrb_Acknowledged_Message, handler);
		packetHandlers.registerHandler(ionet::PacketType::Dbrb_Commit_Message, handler);
		packetHandlers.registerHandler(ionet::PacketType::Dbrb_Deliver_Message, handler);
		packetHandlers.registerHandler(ionet::PacketType::Dbrb_Confirm_Deliver_Message, handler);
	}

	void DbrbProcess::setValidationCallback(const ValidationCallback& callback) {
		m_validationCallback = callback;
	}

	void DbrbProcess::setDeliverCallback(const DeliverCallback& callback) {
		m_deliverCallback = callback;
	}

	boost::asio::io_context::strand& DbrbProcess::strand() {
		return m_strand;
	}

	std::shared_ptr<MessageSender> DbrbProcess::messageSender() {
		return m_pMessageSender;
	}

	const View& DbrbProcess::currentView() {
		return m_currentView;
	}

	const View& DbrbProcess::bootstrapView() {
		return m_bootstrapView;
	}

	const ProcessId& DbrbProcess::id() {
		return m_id;
	}

	// Basic operations:

	void DbrbProcess::broadcast(const Payload& payload, std::set<ProcessId> recipients) {
		CATAPULT_LOG(trace) << "[DBRB] BROADCAST: payload " << payload->Type;
		boost::asio::post(m_strand, [pThisWeak = weak_from_this(), payload, broadcastViewData = std::move(recipients)]() {
			CATAPULT_LOG(trace) << "[DBRB] BROADCAST: stranded broadcast call for payload " << payload->Type;
			auto pThis = pThisWeak.lock();
			if (!pThis)
				return;

			View broadcastView{ broadcastViewData };
			if (broadcastView.Data.empty()) {
				CATAPULT_LOG(debug) << "[DBRB] BROADCAST: broadcast view is empty, aborting broadcast";
				return;
			}

			if (!(broadcastView <= pThis->m_currentView)) {
				CATAPULT_LOG(debug) << "[DBRB] BROADCAST: " << broadcastView << " is not a subview of the current view " << pThis->m_currentView << ", aborting broadcast";
				return;
			}

			if (!(pThis->m_bootstrapView <= broadcastView)) {
				CATAPULT_LOG(debug) << "[DBRB] BROADCAST: bootstrap view " << pThis->m_bootstrapView << " is not a subview of the broadcast view " << broadcastView << ", aborting broadcast";
				return;
			}

			if (!broadcastView.isMember(pThis->m_id)) {
				CATAPULT_LOG(debug) << "[DBRB] BROADCAST: not a member of the broadcast view " << broadcastView << ", aborting broadcast";
				return;
			}

			auto payloadHash = CalculatePayloadHash(payload);
			auto& data = pThis->m_broadcastData[payloadHash];
			data.Begin = utils::NetworkTime();
			data.Payload = payload;
			data.BroadcastView = broadcastView;
			data.BootstrapView = pThis->m_bootstrapView;

			CATAPULT_LOG(trace) << "[DBRB] BROADCAST: sending payload " << payload->Type;
			auto pMessage = std::make_shared<PrepareMessage>(pThis->m_id, payload, broadcastView, data.BootstrapView);
			pThis->disseminate(pMessage, pMessage->View.Data, 0);
		});
	}

	void DbrbProcess::processMessage(const Message& message) {
		switch (message.Type) {
			case ionet::PacketType::Dbrb_Prepare_Message: {
				CATAPULT_LOG(trace) << "[DBRB] Received PREPARE message from " << message.Sender;
				onPrepareMessageReceived(dynamic_cast<const PrepareMessage&>(message));
				break;
			}
			case ionet::PacketType::Dbrb_Acknowledged_Declined_Message: {
				CATAPULT_LOG(trace) << "[DBRB] Received ACKNOWLEDGED (DECLINED) message from " << message.Sender;
				onAcknowledgedDeclinedMessageReceived(dynamic_cast<const AcknowledgedDeclinedMessage&>(message));
				break;
			}
			case ionet::PacketType::Dbrb_Acknowledged_Message: {
				CATAPULT_LOG(trace) << "[DBRB] Received ACKNOWLEDGED message from " << message.Sender;
				onAcknowledgedMessageReceived(dynamic_cast<const AcknowledgedMessage&>(message));
				break;
			}
			case ionet::PacketType::Dbrb_Commit_Message: {
				CATAPULT_LOG(trace) << "[DBRB] Received COMMIT message from " << message.Sender;
				onCommitMessageReceived(dynamic_cast<const CommitMessage&>(message));
				break;
			}
			case ionet::PacketType::Dbrb_Deliver_Message: {
				CATAPULT_LOG(trace) << "[DBRB] Received DELIVER message from " << message.Sender;
				onDeliverMessageReceived(dynamic_cast<const DeliverMessage&>(message));
				break;
			}
			case ionet::PacketType::Dbrb_Confirm_Deliver_Message: {
				CATAPULT_LOG(trace) << "[DBRB] Received CONFIRM DELIVER message from " << message.Sender;
				onConfirmDeliverMessageReceived(dynamic_cast<const ConfirmDeliverMessage&>(message));
				break;
			}
			default:
				CATAPULT_LOG(error) << "invalid DBRB message type" << message.Type;
		}
	}

	// Basic private methods:

	void DbrbProcess::disseminate(const std::shared_ptr<Message>& pMessage, std::set<ProcessId> recipients, uint64_t delayMillis) {
		CATAPULT_LOG(trace) << "[DBRB] disseminating message " << pMessage->Type << " to " << View{ recipients };
		auto pPacket = pMessage->toNetworkPacket();
		for (auto iter = recipients.begin(); iter != recipients.end(); ++iter) {
			if (m_id == *iter) {
				m_pDelayedExecutor->execute(delayMillis, [pThisWeak = weak_from_this(), pMessage]() {
					auto pThis = pThisWeak.lock();
					if (pThis)
						pThis->processMessage(*pMessage);
				});
				recipients.erase(iter);
				break;
			}
		}

		if (delayMillis) {
			m_pDelayedExecutor->execute(delayMillis, [pThisWeak = weak_from_this(), pPacket, recipients]() {
				auto pThis = pThisWeak.lock();
				if (pThis)
					pThis->m_pMessageSender->enqueue(pPacket, false, recipients);
			});
		} else {
			m_pMessageSender->enqueue(pPacket, false, recipients);
		}
	}

	void DbrbProcess::send(const std::shared_ptr<Message>& pMessage, const ProcessId& recipient, uint64_t delayMillis) {
		disseminate(pMessage, std::set<ProcessId>{ recipient }, delayMillis);
	}

	Signature DbrbProcess::sign(const Payload& payload, const View& view) {
		// Forms a hash based on payload and the broadcast view and signs it.
		uint32_t packetPayloadSize = view.packedSize();
		auto pPacket = ionet::CreateSharedPacket<ionet::Packet>(packetPayloadSize);
		auto pBuffer = pPacket->Data();
		Write(pBuffer, view);

		auto hash = CalculateHash({ { reinterpret_cast<const uint8_t*>(payload.get()), payload->Size }, { pPacket->Data(), packetPayloadSize } });
		Signature signature;
		crypto::Sign(m_keyPair, hash, signature);

		return signature;
	}

	bool DbrbProcess::verify(const ProcessId& signer, const Payload& payload, const View& view, const Signature& signature) {
		// Verifies a hash based on payload and current view and checks whether the signature is valid.
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

		if (!(message.View <= m_currentView)) {
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: Aborting message processing (supplied view is not a subview of the current view)";
			return;
		}

		if (!message.View.isMember(m_id)) {
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: Aborting message processing (node is not a participant).";
			return;
		}

		if (!message.View.isMember(message.Sender)) {
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: Aborting message processing (sender is not in supplied view)";
			return;
		}

		if (message.BootstrapView != m_bootstrapView) {
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: Aborting message processing (invalid bootstrap view)\nExpected bootstrap view: " << m_bootstrapView << "\nActual bootstrap view:   " << message.BootstrapView;
			return;
		}

		auto payloadHash = CalculatePayloadHash(message.Payload);
		auto validationResult = m_validationCallback(message.Payload, payloadHash);
		switch (validationResult) {
			case MessageValidationResult::Message_Valid: {
				CATAPULT_LOG(debug) << "[DBRB] PREPARE: Message valid";
				break;
			}
			case MessageValidationResult::Message_Broadcast_Paused: {
				CATAPULT_LOG(debug) << "[DBRB] PREPARE: Broadcast paused";
				auto pMessage = std::make_shared<AcknowledgedDeclinedMessage>(m_id, payloadHash);
				send(pMessage, message.Sender, 0);
				return;
			}
			default: {
				CATAPULT_LOG(debug) << "[DBRB] PREPARE: Aborting message processing (" << validationResult << ")";
				return;
			}
		}

		if (message.Sender != m_id) {
			auto& data = m_broadcastData[payloadHash];
			if (!data.Payload) {
				data.Begin = utils::NetworkTime();
				data.Payload = message.Payload;
				data.BroadcastView = message.View;
				data.BootstrapView = message.BootstrapView;

				CATAPULT_LOG(trace) << "[DBRB] PREPARE: sending payload " << data.Payload->Type;
				auto pMessage = std::make_shared<PrepareMessage>(m_id, data.Payload, data.BroadcastView, data.BootstrapView);
				disseminate(pMessage, pMessage->View.Data, 0);
			}
		}

		CATAPULT_LOG(trace) << "[DBRB] PREPARE: Sending Acknowledged message to " << message.Sender;
		Signature payloadSignature = sign(message.Payload, message.View);
		auto pMessage = std::make_shared<AcknowledgedMessage>(m_id, payloadHash, message.View, payloadSignature);
		send(pMessage, message.Sender, 0);
	}

	void DbrbProcess::onAcknowledgedDeclinedMessageReceived(const AcknowledgedDeclinedMessage& message) {
		auto& data = m_broadcastData[message.PayloadHash];
		if (!data.Payload) {
			CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED (DECLINED): Aborting message processing (no payload)";
			return;
		}

		if (!data.BroadcastView.isMember(message.Sender)) {
			CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED (DECLINED): Aborting message processing (node is not a participant)";
			return;
		}

		CATAPULT_LOG(trace) << "[DBRB] ACKNOWLEDGED (DECLINED): payload " << data.Payload->Type << " from " << message.Sender;

		auto pMessage = std::make_shared<PrepareMessage>(m_id, data.Payload, data.BroadcastView, data.BootstrapView);
		send(pMessage, message.Sender, 500);
	}

	void DbrbProcess::onAcknowledgedMessageReceived(const AcknowledgedMessage& message) {
		if (!message.View.isMember(m_id)) {
			CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: Aborting message processing (node is not a participant).";
			return;
		}

		if (!message.View.isMember(message.Sender)) {
			CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: Aborting message processing (sender is not in supplied view)";
			return;
		}

		auto& data = m_broadcastData[message.PayloadHash];
		if (!data.Payload) {
			CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: Aborting message processing (no payload)";
			return;
		}

		if (message.View != data.BroadcastView) {
			CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: Aborting message processing (supplied view is not the broadcast view)";
			return;
		}

		CATAPULT_LOG(trace) << "[DBRB] ACKNOWLEDGED: payload " << data.Payload->Type << " from " << message.Sender;

		if (!verify(message.Sender, data.Payload, message.View, message.PayloadSignature)) {
			CATAPULT_LOG(warning) << "[DBRB] ACKNOWLEDGED: message with payload " << data.Payload->Type << " from " << message.Sender << " REJECTED: signature is not valid";
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
		CATAPULT_LOG(trace) << "[DBRB] ACKNOWLEDGED: Quorum collected in view " << message.View << ". Payload " << data.Payload->Type;
		data.Certificate.clear();
		const auto& acknowledgedSet = data.QuorumManager.AcknowledgedPayloads[message.View];
		for (const auto& [processId, hash] : acknowledgedSet) {
			auto iter = data.Signatures.find(std::make_pair(message.View, processId));
			if (hash == message.PayloadHash && data.Signatures.end() != iter)
				data.Certificate[processId] = iter->second;
		}


		if (!data.CommitMessageReceived) {
			data.CommitMessageReceived = true;

			CATAPULT_LOG(trace) << "[DBRB] ACKNOWLEDGED: Disseminating Commit message with payload " << data.Payload->Type;
			auto pMessage = std::make_shared<CommitMessage>(m_id, message.PayloadHash, data.Certificate, message.View);
			disseminate(pMessage, message.View.Data, 0);
		}
	}

	void DbrbProcess::onCommitMessageReceived(const CommitMessage& message) {
		if (!message.View.isMember(m_id)) {
			CATAPULT_LOG(debug) << "[DBRB] COMMIT: Aborting message processing (process is not a participant).";
			return;
		}

		if (!message.View.isMember(message.Sender)) {
			CATAPULT_LOG(debug) << "[DBRB] COMMIT: Aborting message processing (sender is not in supplied view)";
			return;
		}

		auto& data = m_broadcastData[message.PayloadHash];
		if (!data.Payload) {
			CATAPULT_LOG(debug) << "[DBRB] COMMIT: Aborting message processing (no payload)";
			return;
		}

		if (message.View != data.BroadcastView) {
			CATAPULT_LOG(debug) << "[DBRB] COMMIT: Aborting message processing (supplied view is not the broadcast view)";
			return;
		}

		CATAPULT_LOG(trace) << "[DBRB] COMMIT: payload " << data.Payload->Type << " from " << message.Sender;

		for (const auto& [signer, signature] : message.Certificate) {
			if (!verify(signer, data.Payload, message.View, signature)) {
				CATAPULT_LOG(warning) << "[DBRB] COMMIT: message with payload " << data.Payload->Type << " from " << message.Sender << " is REJECTED: signature is not valid";
				return;
			}
		}

		if (!data.CommitMessageReceived) {
			data.CommitMessageReceived = true;

			CATAPULT_LOG(trace) << "[DBRB] COMMIT: Disseminating Commit message with payload " << data.Payload->Type;
			auto pMessage = std::make_shared<CommitMessage>(m_id, message.PayloadHash, message.Certificate, message.View);
			disseminate(pMessage, message.View.Data, 0);
		}

		CATAPULT_LOG(trace) << "[DBRB] COMMIT: Sending Deliver message with payload " << data.Payload->Type << " to " << message.Sender;
		auto pMessage = std::make_shared<DeliverMessage>(m_id, message.PayloadHash, message.View);
		send(pMessage, message.Sender, 0);
	}

	void DbrbProcess::onDeliverMessageReceived(const DeliverMessage& message) {
		if (!message.View.isMember(m_id)) {
			CATAPULT_LOG(debug) << "[DBRB] DELIVER: Aborting message processing (node is not a participant)";
			return;
		}

		if (!message.View.isMember(message.Sender)) {
			CATAPULT_LOG(debug) << "[DBRB] DELIVER: Aborting message processing (sender is not in supplied view)";
			return;
		}

		auto& data = m_broadcastData[message.PayloadHash];
		if (!data.Payload) {
			CATAPULT_LOG(debug) << "[DBRB] DELIVER: Aborting message processing (no payload)";
			return;
		}

		if (message.View != data.BroadcastView) {
			CATAPULT_LOG(debug) << "[DBRB] DELIVER: Aborting message processing (supplied view is not the broadcast view)";
			return;
		}

		CATAPULT_LOG(trace) << "[DBRB] DELIVER: payload " << data.Payload->Type << " from " << message.Sender;

		auto quorumCollected = data.QuorumManager.update(message, data.Payload->Type);
		if (quorumCollected && data.BootstrapView.isMember(m_id)) {
			CATAPULT_LOG(trace) << "[DBRB] DELIVER: Disseminating Confirm Deliver message with payload " << data.Payload->Type;
			auto pMessage = std::make_shared<ConfirmDeliverMessage>(m_id, message.PayloadHash, message.View);
			disseminate(pMessage, message.View.Data, 0);
		}
	}

	void DbrbProcess::onConfirmDeliverMessageReceived(const ConfirmDeliverMessage& message) {
		if (!message.View.isMember(m_id)) {
			CATAPULT_LOG(debug) << "[DBRB] CONFIRM DELIVER: Aborting message processing (node is not a participant)";
			return;
		}

		if (!message.View.isMember(message.Sender)) {
			CATAPULT_LOG(debug) << "[DBRB] CONFIRM DELIVER: Aborting message processing (sender is not in supplied view)";
			return;
		}

		auto& data = m_broadcastData[message.PayloadHash];
		if (!data.Payload) {
			CATAPULT_LOG(debug) << "[DBRB] CONFIRM DELIVER: Aborting message processing (no payload)";
			return;
		}

		if (message.View != data.BroadcastView) {
			CATAPULT_LOG(debug) << "[DBRB] CONFIRM DELIVER: Aborting message processing (supplied view is not the broadcast view)";
			return;
		}

		CATAPULT_LOG(trace) << "[DBRB] CONFIRM DELIVER: payload " << data.Payload->Type << " from " << message.Sender;

		bool quorumCollected = data.QuorumManager.update(message, data.BootstrapView, data.Payload->Type);
		if (quorumCollected) {
			CATAPULT_LOG(debug) << "[DBRB] CONFIRM DELIVER: delivering payload " << data.Payload->Type;
			m_deliverCallback(data.Payload);

			CATAPULT_LOG(debug) << "[DBRB] BROADCAST: operation took " << (utils::NetworkTime().unwrap() - data.Begin.unwrap()) << " ms to deliver " << data.Payload->Type;
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

	bool DbrbProcess::updateView(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder, const Timestamp& now, const Height& height) {
		m_pDelayedExecutor->cancel();

		auto view = View{ m_dbrbViewFetcher.getView(now) };
		m_dbrbViewFetcher.logAllProcesses();
		m_dbrbViewFetcher.logView(view.Data);
		auto isTemporaryProcess = view.isMember(m_id);

		CATAPULT_LOG(debug) << "[DBRB] getting config at height " << height;
		const auto& config = pConfigHolder->Config(height).Network;
		auto bootstrapView = View{ config.DbrbBootstrapProcesses };
		if (bootstrapView.Data.empty()) {
			CATAPULT_LOG(debug) << "[DBRB] no bootstrap nodes, getting config at height " << height + Height(1);
			bootstrapView = View{ pConfigHolder->Config(height + Height(1)).Network.DbrbBootstrapProcesses };
		}
		if (bootstrapView.Data.empty())
			CATAPULT_THROW_RUNTIME_ERROR("Bootstrap view is empty")
		auto isBootstrapProcess = bootstrapView.isMember(m_id);

		boost::asio::post(m_strand, [pThisWeak = weak_from_this(), pConfigHolder, now, view, isTemporaryProcess, isBootstrapProcess, gracePeriod = Timestamp(config.DbrbRegistrationGracePeriod.millis()), bootstrapView]() {
			auto pThis = pThisWeak.lock();
			if (!pThis)
				return;

			pThis->m_pMessageSender->clearQueue();
			pThis->m_broadcastData.clear();

			pThis->m_currentView = view;
			pThis->m_bootstrapView = bootstrapView;
			auto bootstrapViewCopy = bootstrapView;
			pThis->m_currentView.merge(bootstrapViewCopy);
			CATAPULT_LOG(debug) << "[DBRB] Current view (" << pThis->m_currentView.Data.size() << ") is now set to " << pThis->m_currentView;

			pThis->m_pMessageSender->findNodes(pThis->m_currentView.Data);

			bool isRegistrationRequired = false;
			if (!isTemporaryProcess && !isBootstrapProcess && (pThis->m_dbrbViewFetcher.getBanPeriod(pThis->m_id) == BlockDuration(0))) {
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
		});

		return isTemporaryProcess || isBootstrapProcess;
	}
}}