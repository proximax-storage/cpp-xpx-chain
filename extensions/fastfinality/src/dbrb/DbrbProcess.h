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
			std::weak_ptr<net::PacketWriters> pWriters,
			const net::PacketIoPickerContainer& packetIoPickers,
			const ionet::Node& thisNode,
			const crypto::KeyPair& keyPair,
			const std::shared_ptr<thread::IoThreadPool>& pPool,
			TransactionSender&& transactionSender,
			const dbrb::DbrbViewFetcher& dbrbViewFetcher,
			chain::TimeSupplier timeSupplier,
			const supplier<Height>& chainHeightSupplier);

	private:
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

		boost::asio::io_context::strand m_strand;

		TransactionSender m_transactionSender;

		const dbrb::DbrbViewFetcher& m_dbrbViewFetcher;

		chain::TimeSupplier m_timeSupplier;

		Timestamp m_lastRegistrationTxTime;

		supplier<Height> m_chainHeightSupplier;

	public:
		/// Broadcast arbitrary \c payload into the system.
		void broadcast(const Payload&);

		void processMessage(const Message& message);

	public:
		void registerPacketHandlers(ionet::ServerPacketHandlers& packetHandlers);
		void setDeliverCallback(const DeliverCallback& callback);
		void updateView(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

		NodeRetreiver& nodeRetreiver();

	private:
		void disseminate(const std::shared_ptr<Message>& pMessage, std::set<ProcessId> recipients);
		void send(const std::shared_ptr<Message>& pMessage, const ProcessId& recipient);

		Signature sign(const Payload&);
		static bool verify(const ProcessId&, const Payload&, const View&, const Signature&);

		void onPrepareMessageReceived(const PrepareMessage&);
		void onAcknowledgedMessageReceived(const AcknowledgedMessage&);
		void onCommitMessageReceived(const CommitMessage&);
		void onDeliverMessageReceived(const DeliverMessage&);

		void onAcknowledgedQuorumCollected(const AcknowledgedMessage&);
		void onDeliverQuorumCollected(const Payload&, const ProcessId&);
	};
}}