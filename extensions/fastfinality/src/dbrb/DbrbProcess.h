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

namespace catapult { namespace thread { class IoThreadPool; }}

namespace catapult { namespace dbrb {

	/// Class representing DBRB process.
	class DbrbProcess : public std::enable_shared_from_this<DbrbProcess>, public utils::NonCopyable {
	public:
		using DeliverCallback = consumer<const Payload&>;

	public:
		explicit DbrbProcess(
			const std::weak_ptr<net::PacketWriters>& pWriters,
			const net::PacketIoPickerContainer& packetIoPickers,
			const ionet::Node& thisNode,
			const crypto::KeyPair& keyPair,
			std::shared_ptr<thread::IoThreadPool> pPool,
			std::shared_ptr<TransactionSender> pTransactionSender,
			const dbrb::DbrbViewFetcher& dbrbViewFetcher);

	protected:
		/// Process identifier.
		ProcessId m_id;

		/// This node.
		SignedNode m_node;

		/// Current view.
		View m_currentView;

		std::map<Hash256, BroadcastData> m_broadcastData;

		NetworkPacketConverter m_converter;

		DeliverCallback m_deliverCallback;

		const crypto::KeyPair& m_keyPair;

		NodeRetreiver m_nodeRetreiver;

		std::shared_ptr<MessageSender> m_pMessageSender;

		std::shared_ptr<thread::IoThreadPool> m_pPool;

		boost::asio::io_context::strand m_strand;

		std::shared_ptr<TransactionSender> m_pTransactionSender;

		const dbrb::DbrbViewFetcher& m_dbrbViewFetcher;

	public:
		/// Broadcast arbitrary \c payload into the system.
		virtual void broadcast(const Payload&);
		virtual void processMessage(const Message& message);
		virtual bool updateView(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder, const Timestamp& now, const Height& height, bool registerSelf);

	public:
		void registerPacketHandlers(ionet::ServerPacketHandlers& packetHandlers);
		void setDeliverCallback(const DeliverCallback& callback);

	public:
		NodeRetreiver& nodeRetreiver();
		boost::asio::io_context::strand& strand();
		MessageSender& messageSender();
		const View& currentView();

	private:
		virtual void disseminate(const std::shared_ptr<Message>& pMessage, std::set<ProcessId> recipients);
		virtual void send(const std::shared_ptr<Message>& pMessage, const ProcessId& recipient);

		Signature sign(const Payload&);
		static bool verify(const ProcessId&, const Payload&, const View&, const Signature&);

		void onPrepareMessageReceived(const PrepareMessage&);
		virtual void onAcknowledgedMessageReceived(const AcknowledgedMessage&);
		virtual void onCommitMessageReceived(const CommitMessage&);
		void onDeliverMessageReceived(const DeliverMessage&);

		void onAcknowledgedQuorumCollected(const AcknowledgedMessage&);
		void onDeliverQuorumCollected(const Payload&, const ProcessId&);
	};
}}