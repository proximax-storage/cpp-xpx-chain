/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/dbrb/Messages.h"
#include "MessageSender.h"
#include "TransactionSender.h"
#include "catapult/functions.h"
#include "catapult/net/PacketWriters.h"
#include "catapult/ionet/PacketHandlers.h"

namespace catapult { namespace thread { class IoThreadPool; }}

namespace catapult { namespace dbrb {

	/// Struct that encapsulates all necessary quorum counters and their update methods.
	struct QuorumManager {
	public:
		/// Maps views to sets of pairs of respective process IDs and payload hashes received from Acknowledged messages.
		std::map<View, std::set<std::pair<ProcessId, Hash256>>> AcknowledgedPayloads;

		/// Maps views to sets of process IDs ready for delivery.
		std::map<View, std::set<ProcessId>> DeliveredProcesses;

		/// Overloaded methods for updating respective counters.
		/// Returns whether the quorum has just been collected on this update.
		bool update(AcknowledgedMessage message) {
			CATAPULT_LOG(debug) << "[DBRB] QUORUM: Received ACKNOWLEDGED message in view " << message.View << ".";
			auto& set = AcknowledgedPayloads[message.View];
			const auto& payloadHash = message.PayloadHash;
			set.emplace(message.Sender, payloadHash);

			const auto acknowledgedCount = std::count_if(set.begin(), set.end(), [&payloadHash](const auto& pair){ return pair.second == payloadHash; });

			const auto triggered = (acknowledgedCount == message.View.quorumSize());
			CATAPULT_LOG(debug) << "[DBRB] QUORUM: ACK quorum status is " << acknowledgedCount << "/" << message.View.quorumSize() << (triggered ? " (TRIGGERED)." : " (NOT triggered).");

			return triggered;
		};

		bool update(const DeliverMessage& message) {
			auto& set = DeliveredProcesses[message.View];
			if (set.emplace(message.Sender).second) {
				bool triggered = set.size() == message.View.quorumSize();
				CATAPULT_LOG(debug) << "[DBRB] QUORUM: DELIVER quorum status is " << set.size() << "/" << message.View.quorumSize() << (triggered ? " (TRIGGERED)." : " (NOT triggered).");
				return triggered;
			} else {
				return false;
			}
		};
	};

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

		/// Quorum manager.
		QuorumManager m_quorumManager;

		/// Current view.
		View m_currentView;

		struct BroadcastData {
			/// The sender of the data.
			ProcessId Sender;

			/// Payload allowed to be acknowledged. If empty, any payload can be acknowledged.
			dbrb::Payload Payload;

			/// Map that maps views and process IDs to signatures received from respective Acknowledged messages.
			std::map<std::pair<View, ProcessId>, Signature> Signatures;

			/// Message certificate; map that maps process IDs to signatures received from them.
			/// Empty when the process starts working.
			CertificateType Certificate;

			/// View in which message certificate was collected.
			View CertificateView;

			/// Quorum manager.
			dbrb::QuorumManager QuorumManager;

			/// Whether any commit message received.
			bool CommitMessageReceived = false;

			Timestamp Begin;
		};

		std::map<Hash256, BroadcastData> m_broadcastData;

		/// State of the process.
		ProcessState m_state;

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