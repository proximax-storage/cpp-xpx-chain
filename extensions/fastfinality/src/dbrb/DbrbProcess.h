/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BroadcastData.h"
#include "MessageSender.h"
#include "QuorumManager.h"
#include "TransactionSender.h"
#include "catapult/dbrb/Messages.h"
#include "catapult/functions.h"
#include "catapult/ionet/PacketHandlers.h"
#include "catapult/net/PacketWriters.h"
#include "catapult/types.h"

namespace catapult {
	namespace dbrb { class DelayedExecutor; }
	namespace thread { class IoThreadPool; }
}

namespace catapult { namespace dbrb {

	/// Class representing DBRB process.
	class DbrbProcess : public std::enable_shared_from_this<DbrbProcess>, public utils::NonCopyable {
	public:
		DbrbProcess(
			const crypto::KeyPair& keyPair,
			std::shared_ptr<MessageSender> pMessageSender,
			std::shared_ptr<thread::IoThreadPool> pPool,
			std::shared_ptr<TransactionSender> pTransactionSender,
			const dbrb::DbrbViewFetcher& dbrbViewFetcher);

	public:
		/// Broadcast arbitrary \c payload into the system.
		virtual void broadcast(const Payload&, std::set<ProcessId> recipients);
		virtual void processMessage(const Message& message);
		virtual bool updateView(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder, const Timestamp& now, const Height& height);

	public:
		void registerPacketHandlers(ionet::ServerPacketHandlers& packetHandlers);
		void setValidationCallback(const ValidationCallback& callback);
		void setDeliverCallback(const DeliverCallback& callback);

	public:
		boost::asio::io_context::strand& strand();
		std::shared_ptr<MessageSender> messageSender();
		const View& currentView();
		const View& bootstrapView();
		const ProcessId& id();

	protected:
		virtual void disseminate(const std::shared_ptr<Message>& pMessage, std::set<ProcessId> recipients, uint64_t delayMillis);
		virtual void send(const std::shared_ptr<Message>& pMessage, const ProcessId& recipient, uint64_t delayMillis);

		Signature sign(const Payload& payload, const View& view);
		static bool verify(const ProcessId&, const Payload&, const View&, const Signature&);

		void onPrepareMessageReceived(const PrepareMessage&);
		void onAcknowledgedDeclinedMessageReceived(const AcknowledgedDeclinedMessage&);
		virtual void onAcknowledgedMessageReceived(const AcknowledgedMessage&);
		virtual void onCommitMessageReceived(const CommitMessage&);
		void onDeliverMessageReceived(const DeliverMessage&);
		void onConfirmDeliverMessageReceived(const ConfirmDeliverMessage&);

		void onAcknowledgedQuorumCollected(const AcknowledgedMessage&);

	protected:
		const crypto::KeyPair& m_keyPair;
		ProcessId m_id;
		View m_currentView;
		View m_bootstrapView;
		std::map<Hash256, BroadcastData> m_broadcastData;
		NetworkPacketConverter m_converter;
		ValidationCallback m_validationCallback;
		DeliverCallback m_deliverCallback;
		std::shared_ptr<MessageSender> m_pMessageSender;
		std::shared_ptr<thread::IoThreadPool> m_pPool;
		boost::asio::io_context::strand m_strand;
		std::shared_ptr<TransactionSender> m_pTransactionSender;
		const dbrb::DbrbViewFetcher& m_dbrbViewFetcher;
		std::shared_ptr<DelayedExecutor> m_pDelayedExecutor;
	};
}}