/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ShardedDbrbProcess.h"
#include "catapult/crypto/Signer.h"
#include "catapult/ionet/NetworkNode.h"
#include "catapult/ionet/PacketPayload.h"
#include "catapult/thread/IoThreadPool.h"
#include "catapult/utils/NetworkTime.h"
#include <utility>


namespace catapult { namespace dbrb {

	ShardedDbrbProcess::ShardedDbrbProcess(
		const crypto::KeyPair& keyPair,
		std::shared_ptr<MessageSender> pMessageSender,
		const std::shared_ptr<thread::IoThreadPool>& pPool,
		std::shared_ptr<TransactionSender> pTransactionSender,
		const dbrb::DbrbViewFetcher& dbrbViewFetcher,
		size_t shardSize)
			: m_keyPair(keyPair)
			, m_id(keyPair.publicKey())
			, m_pMessageSender(std::move(pMessageSender))
			, m_strand(pPool->ioContext())
			, m_pTransactionSender(std::move(pTransactionSender))
			, m_dbrbViewFetcher(dbrbViewFetcher)
			, m_shardSize(shardSize)
	{}

	void ShardedDbrbProcess::registerPacketHandlers(ionet::ServerPacketHandlers& packetHandlers) {
		auto handler = [pThisWeak = weak_from_this(), &converter = m_converter, &strand = m_strand](const auto& packet, auto& context) {
			auto pMessage = converter.toMessage(packet);
			boost::asio::post(strand, [pThisWeak, pMessage]() {
				auto pThis = pThisWeak.lock();
				if (pThis)
					pThis->processMessage(*pMessage);
			});
		};
		packetHandlers.registerHandler(ionet::PacketType::Dbrb_Shard_Prepare_Message, handler);
		packetHandlers.registerHandler(ionet::PacketType::Dbrb_Shard_Acknowledged_Message, handler);
		packetHandlers.registerHandler(ionet::PacketType::Dbrb_Shard_Commit_Message, handler);
		packetHandlers.registerHandler(ionet::PacketType::Dbrb_Shard_Deliver_Message, handler);
	}

	void ShardedDbrbProcess::setValidationCallback(const ValidationCallback& callback) {
		m_validationCallback = callback;
	}

	void ShardedDbrbProcess::setDeliverCallback(const DeliverCallback& callback) {
		m_deliverCallback = callback;
	}

	boost::asio::io_context::strand& ShardedDbrbProcess::strand() {
		return m_strand;
	}

	std::shared_ptr<MessageSender> ShardedDbrbProcess::messageSender() {
		return m_pMessageSender;
	}

	const View& ShardedDbrbProcess::currentView() const {
		return m_currentView;
	}

	const ProcessId& ShardedDbrbProcess::id() const  {
		return m_id;
	}

	size_t ShardedDbrbProcess::shardSize() const {
		return m_shardSize;
	}

	// Basic operations:

	void ShardedDbrbProcess::broadcast(const Payload& payload, ViewData recipients) {
		CATAPULT_LOG(debug) << "[DBRB] BROADCAST: payload " << payload->Type;
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

			if (broadcastView > pThis->m_currentView) {
				CATAPULT_LOG(debug) << "[DBRB] BROADCAST: " << broadcastView << " is not a subview of the current view " << pThis->m_currentView << ", aborting broadcast";
				return;
			}

			if (!broadcastView.isMember(pThis->m_id)) {
				CATAPULT_LOG(debug) << "[DBRB] BROADCAST: not a member of the current view " << pThis->m_currentView << ", aborting broadcast";
				return;
			}

			auto payloadHash = CalculatePayloadHash(payload);
			auto& data = pThis->m_broadcastData[payloadHash];
			data.Begin = utils::NetworkTime();
			data.Payload = payload;
			data.Broadcaster = pThis->m_id;
			data.BroadcastView = std::move(broadcastView);
			data.SubTreeView = data.BroadcastView;
			data.ParentShardQuorumSize = 1;
			data.NetworkQuorumSize = data.BroadcastView.quorumSize();
			data.ChildShardQuorumSize = data.NetworkQuorumSize;
			auto reachableNodes = data.SubTreeView.Data;
			reachableNodes.erase(pThis->m_id);
			auto unreachableNodes = pThis->m_pMessageSender->getUnreachableNodes(reachableNodes);
			data.Tree = CreateDbrbTreeView(reachableNodes, unreachableNodes, pThis->m_id, pThis->m_shardSize);
			auto signature = pThis->sign(ionet::PacketType::Dbrb_Shard_Acknowledged_Message, payload, data.Tree);
			data.AcknowledgeCertificate.emplace(pThis->m_id, signature);
			data.Shard = CreateDbrbShard(data.Tree, pThis->m_id, pThis->m_shardSize);
			if (!data.Shard.Initialized) {
				CATAPULT_LOG(error) << "[DBRB] BROADCAST: failed to create shard, aborting broadcast";
				return;
			}

			CATAPULT_LOG(trace) << "[DBRB] BROADCAST: sending payload " << payload->Type;
			signature = pThis->sign(ionet::PacketType::Dbrb_Shard_Prepare_Message, payload, data.Tree);
			auto pMessage = std::make_shared<ShardPrepareMessage>(pThis->m_id, payload, data.Tree, signature);
			pThis->disseminate(pMessage, data.Shard.Children);
		});
	}

	// Basic private methods:

	void ShardedDbrbProcess::disseminate(const std::shared_ptr<Message>& pMessage, ViewData recipients) {
		CATAPULT_LOG(trace) << "[DBRB] disseminating message " << pMessage->Type << " to " << View{ recipients };
		auto pPacket = pMessage->toNetworkPacket();
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

		if (!recipients.empty())
			m_pMessageSender->enqueue(pPacket, false, recipients);
	}

	void ShardedDbrbProcess::send(const std::shared_ptr<Message>& pMessage, const ProcessId& recipient) {
		disseminate(pMessage, ViewData{ recipient });
	}

	Signature ShardedDbrbProcess::sign(ionet::PacketType type, const Payload& payload, const DbrbTreeView& view) {
		// Forms a hash based on message type, payload and the broadcast view and signs it.
		uint32_t packetPayloadSize = 2 * sizeof(uint32_t) + view.size() * ProcessId_Size;
		auto pPacket = ionet::CreateSharedPacket<ionet::Packet>(packetPayloadSize);
		auto pBuffer = pPacket->Data();
		Write(pBuffer, static_cast<uint32_t>(type));
		Write(pBuffer, view);

		auto hash = CalculateHash({ { reinterpret_cast<const uint8_t*>(payload.get()), payload->Size }, { pPacket->Data(), packetPayloadSize } });
		Signature signature;
		crypto::Sign(m_keyPair, hash, signature);

		return signature;
	}

	bool ShardedDbrbProcess::verify(ionet::PacketType type, const ProcessId& signer, const Payload& payload, const DbrbTreeView& view, const Signature& signature) {
		// Verifies a hash based on message type, payload and current view and checks whether the signature is valid.
		uint32_t packetPayloadSize = 2 * sizeof(uint32_t) + view.size() * ProcessId_Size;
		auto pPacket = ionet::CreateSharedPacket<ionet::Packet>(packetPayloadSize);
		auto pBuffer = pPacket->Data();
		Write(pBuffer, static_cast<uint32_t>(type));
		Write(pBuffer, view);

		auto hash = CalculateHash({ { reinterpret_cast<const uint8_t*>(payload.get()), payload->Size }, { pPacket->Data(), packetPayloadSize } });

		bool res = crypto::Verify(signer, hash, signature);
		return res;
	}

	// Message callbacks:

	void ShardedDbrbProcess::processMessage(const Message& message) {
		switch (message.Type) {
		case ionet::PacketType::Dbrb_Shard_Prepare_Message: {
			CATAPULT_LOG(trace) << "[DBRB] Received PREPARE message from " << message.Sender;
			onPrepareMessageReceived(dynamic_cast<const ShardPrepareMessage&>(message));
			break;
		}
		case ionet::PacketType::Dbrb_Shard_Acknowledged_Message: {
			CATAPULT_LOG(trace) << "[DBRB] Received ACKNOWLEDGED message from " << message.Sender;
			onAcknowledgedMessageReceived(dynamic_cast<const ShardAcknowledgedMessage&>(message));
			break;
		}
		case ionet::PacketType::Dbrb_Shard_Commit_Message: {
			CATAPULT_LOG(trace) << "[DBRB] Received COMMIT message from " << message.Sender;
			onCommitMessageReceived(dynamic_cast<const ShardCommitMessage&>(message));
			break;
		}
		case ionet::PacketType::Dbrb_Shard_Deliver_Message: {
			CATAPULT_LOG(trace) << "[DBRB] Received DELIVER message from " << message.Sender;
			onDeliverMessageReceived(dynamic_cast<const ShardDeliverMessage&>(message));
			break;
		}
		default:
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid DBRB message type", message.Type)
		}
	}

	void ShardedDbrbProcess::onPrepareMessageReceived(const ShardPrepareMessage& message) {
		CATAPULT_LOG(trace) << "[DBRB] PREPARE: received payload " << message.Payload->Type << " from " << message.Sender;
		if (m_validationCallback(message.Payload) != MessageValidationResult::Message_Valid) {
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: Aborting message processing (message invalid)";
			return;
		}

		View view;
		for (const auto& id : message.View)
			view.Data.emplace(id);

		if (view > m_currentView) {
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: Aborting message processing (supplied view is not a subview of the current view)";
			return;
		}

		if (!view.isMember(m_id)) {
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: Aborting message processing (node is not a participant).";
			return;
		}

		auto payloadHash = CalculatePayloadHash(message.Payload);
		auto& data = m_broadcastData[payloadHash];
		if (data.Payload) {
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: message already processed";
			return;
		}

		data.Shard = CreateDbrbShard(message.View, m_id, m_shardSize);
		if (!data.Shard.Initialized) {
			m_broadcastData.erase(payloadHash);
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: Aborting message processing (failed to create shard)";
			return;
		}

		if (data.Shard.Parent != message.Sender) {
			m_broadcastData.erase(payloadHash);
			CATAPULT_LOG(debug) << "[DBRB] PREPARE: Aborting message processing (sender is not parent)";
			return;
		}

		data.Payload = message.Payload;
		data.Broadcaster = message.View[0];
		if (!verify(ionet::PacketType::Dbrb_Shard_Prepare_Message, data.Broadcaster, data.Payload, message.View, message.BroadcasterSignature)) {
			CATAPULT_LOG(warning) << "[DBRB] PREPARE: message with payload " << data.Payload->Type << " from " << message.Sender << " REJECTED: invalid broadcaster signature";
			return;
		}

		data.Begin = utils::NetworkTime();
		data.Tree = message.View;

		for (const auto& id : message.View)
			data.BroadcastView.Data.emplace(id);

		data.SubTreeView.Data.emplace(m_id);
		for (const auto& [_, childView] : data.Shard.ChildViews) {
			for (const auto& id : childView)
				data.SubTreeView.Data.emplace(id);
		}

		data.NetworkQuorumSize = View::quorumSize(data.Tree.size());
		data.ParentShardQuorumSize = View::quorumSize(data.Tree.size() - data.SubTreeView.Data.size() + 1);
		data.ChildShardQuorumSize = data.SubTreeView.quorumSize();

		auto signature = sign(ionet::PacketType::Dbrb_Shard_Acknowledged_Message, message.Payload, message.View);
		data.AcknowledgeCertificate.emplace(m_id, signature);
		if (!data.Acknowledged && data.AcknowledgeCertificate.size() >= data.ChildShardQuorumSize) {
			CATAPULT_LOG(trace) << "[DBRB] PREPARE: Sending Acknowledged message to sender " << message.Sender;
			data.Acknowledged = true;
			auto pMessage = std::make_shared<ShardAcknowledgedMessage>(m_id, payloadHash, std::move(data.AcknowledgeCertificate));
			send(pMessage, message.Sender);
		}

		if (!data.Shard.Children.empty()) {
			CATAPULT_LOG(trace) << "[DBRB] PREPARE: Sending Prepare message to children";
			auto pMessage = std::make_shared<ShardPrepareMessage>(m_id, message.Payload, message.View, message.BroadcasterSignature);
			disseminate(pMessage, data.Shard.Children);
		}
	}

	void ShardedDbrbProcess::onAcknowledgedMessageReceived(const ShardAcknowledgedMessage& message) {
		auto& data = m_broadcastData[message.PayloadHash];
		if (!data.Payload) {
			CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: Aborting message processing (no payload)";
			return;
		}

		auto iter = data.Shard.ChildViews.find(message.Sender);
		if (iter == data.Shard.ChildViews.cend()) {
			CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: Aborting message processing (invalid sender)";
			return;
		}

		const auto& childView = iter->second;
		for (const auto& [signer, signature] : message.Certificate) {
			if (childView.find(signer) == childView.cend()) {
				CATAPULT_LOG(warning) << "[DBRB] ACKNOWLEDGED: message with payload " << data.Payload->Type << " from " << message.Sender << " is REJECTED: invalid signer " << signer;
				return;
			}

			if (!verify(ionet::PacketType::Dbrb_Shard_Acknowledged_Message, signer, data.Payload, data.Tree, signature)) {
				CATAPULT_LOG(warning) << "[DBRB] ACKNOWLEDGED: message with payload " << data.Payload->Type << " from " << message.Sender << " is REJECTED: invalid signature, signer " << signer << ", signature " << signature;
				return;
			}

			data.AcknowledgeCertificate.emplace(signer, signature);
		}

		CATAPULT_LOG(trace) << "[DBRB] ACKNOWLEDGED: payload " << data.Payload->Type << " from " << message.Sender;

		if (!data.Acknowledged && data.AcknowledgeCertificate.size() < data.ChildShardQuorumSize)
			return;

		data.Acknowledged = true;
		if (m_id == data.Broadcaster) {
			if (!data.CommitMessageSent) {
				data.CommitMessageSent = true;
				auto signature = sign(ionet::PacketType::Dbrb_Shard_Deliver_Message, data.Payload, data.Tree);
				data.ParentShardDeliverCertificate.emplace(m_id, signature);
				data.ChildShardDeliverCertificate.emplace(m_id, signature);

				for (const auto& id : data.Shard.Children)
					data.ParentShardDeliverCertificateRecipients.emplace(id, DeliverCertificate{ false, false, data.ParentShardDeliverCertificate });

				CATAPULT_LOG(trace) << "[DBRB] ACKNOWLEDGED: Disseminating Commit message with payload " << data.Payload->Type;
				auto pMessage = std::make_shared<ShardCommitMessage>(m_id, message.PayloadHash, std::move(data.AcknowledgeCertificate));
				disseminate(pMessage, data.Shard.Children);
			}
		} else {
			CATAPULT_LOG(trace) << "[DBRB] ACKNOWLEDGED: sending Acknowledged message with payload " << data.Payload->Type;
			auto pMessage = std::make_shared<ShardAcknowledgedMessage>(m_id, message.PayloadHash, std::move(data.AcknowledgeCertificate));
			send(pMessage, data.Shard.Parent);
		}
	}

	void ShardedDbrbProcess::onCommitMessageReceived(const ShardCommitMessage& message) {
		auto& data = m_broadcastData[message.PayloadHash];
		if (!data.Payload) {
			CATAPULT_LOG(debug) << "[DBRB] COMMIT: Aborting message processing (no payload)";
			return;
		}

		if (data.Shard.Neighbours.find(message.Sender) == data.Shard.Neighbours.cend()) {
			CATAPULT_LOG(debug) << "[DBRB] COMMIT: Aborting message processing (invalid sender)";
			return;
		}

		if (message.Certificate.size() < data.NetworkQuorumSize) {
			CATAPULT_LOG(debug) << "[DBRB] COMMIT: Aborting message processing (invalid certificate)";
			return;
		}

		for (const auto& [signer, signature] : message.Certificate) {
			if (!data.BroadcastView.isMember(signer)) {
				CATAPULT_LOG(warning) << "[DBRB] COMMIT: message with payload " << data.Payload->Type << " from " << message.Sender << " is REJECTED: invalid signer " << signer;
				return;
			}

			if (!verify(ionet::PacketType::Dbrb_Shard_Acknowledged_Message, signer, data.Payload, data.Tree, signature)) {
				CATAPULT_LOG(warning) << "[DBRB] COMMIT: message with payload " << data.Payload->Type << " from " << message.Sender << " is REJECTED: invalid signature, signer " << signer << ", signature " << signature;
				return;
			}
		}

		CATAPULT_LOG(trace) << "[DBRB] COMMIT: payload " << data.Payload->Type << " from " << message.Sender;

		if (!data.CommitMessageSent) {
			data.CommitMessageSent = true;
			auto signature = sign(ionet::PacketType::Dbrb_Shard_Deliver_Message, data.Payload, data.Tree);
			data.ParentShardDeliverCertificate.emplace(m_id, signature);
			data.ChildShardDeliverCertificate.emplace(m_id, signature);

			data.ChildShardDeliverCertificateRecipients.emplace(data.Shard.Parent, DeliverCertificate{ false, false, data.ChildShardDeliverCertificate });
			for (const auto& id : data.Shard.Siblings)
				data.ChildShardDeliverCertificateRecipients.emplace(id, DeliverCertificate{ false, false, data.ChildShardDeliverCertificate });
			for (const auto& id : data.Shard.Children)
				data.ParentShardDeliverCertificateRecipients.emplace(id, DeliverCertificate{ false, false, data.ParentShardDeliverCertificate });

			CATAPULT_LOG(trace) << "[DBRB] COMMIT: Disseminating Commit message with payload " << data.Payload->Type;
			auto pMessage = std::make_shared<ShardCommitMessage>(m_id, message.PayloadHash, message.Certificate);
			disseminate(pMessage, data.Shard.Neighbours);
		}

		DeliverCertificate* pCertificate;
		if (message.Sender == data.Shard.Parent || data.Shard.Siblings.find(message.Sender) != data.Shard.Siblings.cend()) {
			pCertificate = &data.ChildShardDeliverCertificateRecipients.at(message.Sender);
			if (!pCertificate->QuorumCollected)
				pCertificate->QuorumCollected = data.ChildShardDeliverCertificate.size() >= data.ChildShardQuorumSize;
		} else {
			pCertificate = &data.ParentShardDeliverCertificateRecipients.at(message.Sender);
			if (!pCertificate->QuorumCollected) {
				auto networkQuorumCollected = (data.ParentShardDeliverCertificate.size() + data.ChildShardDeliverCertificate.size() >= data.NetworkQuorumSize + 1);
				pCertificate->QuorumCollected = networkQuorumCollected || data.ParentShardDeliverCertificate.size() >= data.ParentShardQuorumSize;
			}
		}

		pCertificate->Requested = true;
		if (pCertificate->QuorumCollected && !pCertificate->Certificate.empty()) {
			CATAPULT_LOG(trace) << "[DBRB] COMMIT: sending Deliver message with payload " << data.Payload->Type << " to " << message.Sender;
			auto pMessage = std::make_shared<ShardDeliverMessage>(m_id, message.PayloadHash, std::move(pCertificate->Certificate));
			send(pMessage, message.Sender);
		}
	}

	void ShardedDbrbProcess::onDeliverMessageReceived(const ShardDeliverMessage& message) {
		auto& data = m_broadcastData[message.PayloadHash];
		if (!data.Payload) {
			CATAPULT_LOG(debug) << "[DBRB] DELIVER: Aborting message processing (no payload)";
			return;
		}

		if (data.Shard.Neighbours.find(message.Sender) == data.Shard.Neighbours.cend()) {
			CATAPULT_LOG(debug) << "[DBRB] DELIVER: Aborting message processing (invalid sender)";
			return;
		}

		CATAPULT_LOG(trace) << "[DBRB] DELIVER: payload " << data.Payload->Type << " from " << message.Sender;

		auto* pCertificate = &data.ParentShardDeliverCertificate;
		auto* pRecipients = &data.ParentShardDeliverCertificateRecipients;
		auto* pView = &data.Shard.ParentView;
		if (message.Sender != data.Shard.Parent) {
			if (data.Shard.Siblings.find(message.Sender) != data.Shard.Siblings.cend()) {
				pView = &data.Shard.SiblingViews.at(message.Sender);
			} else {
				pCertificate = &data.ChildShardDeliverCertificate;
				pRecipients = &data.ChildShardDeliverCertificateRecipients;
				pView = &data.Shard.ChildViews.at(message.Sender);
			}
		}

		for (const auto& [signer, signature] : message.Certificate) {
			if (pView->find(signer) == pView->cend()) {
				CATAPULT_LOG(warning) << "[DBRB] DELIVER: message with payload " << data.Payload->Type << " from " << message.Sender << " is REJECTED: invalid signer " << signer;
				return;
			}

			if (!verify(ionet::PacketType::Dbrb_Shard_Deliver_Message, signer, data.Payload, data.Tree, signature)) {
				CATAPULT_LOG(warning) << "[DBRB] DELIVER: message with payload " << data.Payload->Type << " from " << message.Sender << " is REJECTED: invalid signature, signer " << signer << ", signature " << signature;
				return;
			}

			pCertificate->emplace(signer, signature);
			for (auto& [recipient, certificate] : *pRecipients)
				certificate.Certificate.emplace(signer, signature);
		}

		auto networkQuorumCollected = (data.ParentShardDeliverCertificate.size() + data.ChildShardDeliverCertificate.size() >= data.NetworkQuorumSize + 1);
		for (auto& [id, certificate] : data.ParentShardDeliverCertificateRecipients) {
			if (!certificate.QuorumCollected)
				certificate.QuorumCollected = networkQuorumCollected || data.ParentShardDeliverCertificate.size() >= data.ParentShardQuorumSize;

			if (certificate.Requested && certificate.QuorumCollected && !certificate.Certificate.empty()) {
				auto pMessage = std::make_shared<ShardDeliverMessage>(m_id, message.PayloadHash, std::move(certificate.Certificate));
				send(pMessage, id);
			}
		}

		for (auto& [id, certificate] : data.ChildShardDeliverCertificateRecipients) {
			if (!certificate.QuorumCollected)
				certificate.QuorumCollected = data.ChildShardDeliverCertificate.size() >= data.ChildShardQuorumSize;

			if (certificate.Requested && certificate.QuorumCollected && !certificate.Certificate.empty()) {
				auto pMessage = std::make_shared<ShardDeliverMessage>(m_id, message.PayloadHash, std::move(certificate.Certificate));
				send(pMessage, id);
			}
		}

		if (!data.Delivered && networkQuorumCollected) {
			data.Delivered = true;
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

	bool ShardedDbrbProcess::updateView(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder, const Timestamp& now, const Height& height, bool registerSelf) {
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
		auto isBootstrapProcess = bootstrapView.isMember(m_id);

		view.merge(bootstrapView);
		if (view.Data.empty())
			CATAPULT_THROW_RUNTIME_ERROR("no DBRB processes")

		auto shardSize = config.DbrbShardSize;
		boost::asio::post(m_strand, [pThisWeak = weak_from_this(), pConfigHolder, now, view, isTemporaryProcess, isBootstrapProcess, shardSize, gracePeriod = Timestamp(config.DbrbRegistrationGracePeriod.millis()), registerSelf]() {
			auto pThis = pThisWeak.lock();
			if (!pThis)
				return;

			pThis->m_pMessageSender->clearQueue();
			pThis->m_broadcastData.clear();

			pThis->m_pMessageSender->findNodes(view.Data);
			pThis->m_shardSize = shardSize;
			pThis->m_currentView = view;
			CATAPULT_LOG(debug) << "[DBRB] Current view (" << view.Data.size() << ") is now set to " << view;

			if (registerSelf) {
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
			}
		});

		return isTemporaryProcess || isBootstrapProcess;
	}
}}