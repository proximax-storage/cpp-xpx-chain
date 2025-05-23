/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BroadcastData.h"
#include "MessageSender.h"
#include "TransactionSender.h"
#include "catapult/dbrb/DbrbDefinitions.h"
#include "catapult/dbrb/Messages.h"
#include "catapult/functions.h"
#include "catapult/ionet/PacketHandlers.h"
#include "catapult/net/PacketWriters.h"
#include "catapult/types.h"

namespace catapult { namespace thread { class IoThreadPool; }}

namespace catapult { namespace dbrb {

	class ShardedDbrbProcess : public std::enable_shared_from_this<ShardedDbrbProcess>, public utils::NonCopyable {
	public:
		ShardedDbrbProcess(
			const crypto::KeyPair& keyPair,
			std::shared_ptr<MessageSender> pMessageSender,
			const std::shared_ptr<thread::IoThreadPool>& pPool,
			std::shared_ptr<TransactionSender> pTransactionSender,
			const dbrb::DbrbViewFetcher& dbrbViewFetcher,
			size_t shardSize);

	public:
		void broadcast(const Payload& payload, ViewData recipients);
		void processMessage(const Message& message);
		bool updateView(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder, const Timestamp& now, const Height& height);
		void registerDbrbProcess(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder, const Timestamp& now, const Height& height);

	public:
		void registerPacketHandlers(ionet::ServerPacketHandlers& packetHandlers);
		void setValidationCallback(const ValidationCallback& callback);
		void setDeliverCallback(const DeliverCallback& callback);
		void setGetDbrbModeCallback(const GetDbrbModeCallback& callback);

	public:
		boost::asio::io_context::strand& strand();
		std::shared_ptr<MessageSender> messageSender() const;
		const ProcessId& id() const;
		size_t shardSize() const;
		void maybeDeliver();
		void clearData();

	protected:
		void disseminate(const std::shared_ptr<Message>& pMessage, ViewData recipients);
		void send(const std::shared_ptr<Message>& pMessage, const ProcessId& recipient);

		Signature sign(ionet::PacketType type, const Payload& payload, const DbrbTreeView& view) const;
		static bool verify(ionet::PacketType type, const ProcessId&, const Payload&, const DbrbTreeView&, const Signature&);

		void onPrepareMessageReceived(const ShardPrepareMessage&);
		void onAcknowledgedMessageReceived(const ShardAcknowledgedMessage&);
		void onCommitMessageReceived(const ShardCommitMessage&);
		void onDeliverMessageReceived(const ShardDeliverMessage&);

	protected:
		const crypto::KeyPair& m_keyPair;
		ProcessId m_id;
		View m_currentView;
		std::map<Hash256, ShardBroadcastData> m_broadcastData;
		NetworkPacketConverter m_converter;
		ValidationCallback m_validationCallback;
		DeliverCallback m_deliverCallback;
		GetDbrbModeCallback m_getDbrbModeCallback;
		std::shared_ptr<MessageSender> m_pMessageSender;
		boost::asio::io_context::strand m_strand;
		std::shared_ptr<TransactionSender> m_pTransactionSender;
		const dbrb::DbrbViewFetcher& m_dbrbViewFetcher;
		size_t m_shardSize;
	};
}}