/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/dbrb/DbrbDefinitions.h"

namespace catapult {
	namespace ionet {
		class Node;
		class NodeContainer;
		class NodePacketIoPair;
		class Packet;
	}
	namespace net { class PacketWriters; }
	namespace dbrb {
		class DbrbViewFetcher;
		class TransactionSender;
	}
	namespace config { class BlockchainConfigurationHolder; }
}

namespace catapult { namespace dbrb {

	using NodePacketIoPairMap = std::map<ProcessId, ionet::NodePacketIoPair>;

	class MessageSender {
	public:
		using BufferType = std::vector<std::pair<Payload, std::set<ProcessId>>>;

	public:
		virtual ~MessageSender() = default;

		// Message sending
		virtual void enqueue(const Payload& payload, const std::set<ProcessId>& recipients) = 0;
		virtual void clearQueue() = 0;
		virtual ionet::NodePacketIoPair getNodePacketIoPair(const ProcessId& id) = 0;
		virtual void pushNodePacketIoPair(const ProcessId& id, const ionet::NodePacketIoPair& nodePacketIoPair) = 0;

	public:
		// Node discovery
		virtual void requestNodes(const std::set<ProcessId>& requestedIds, const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) = 0;
		virtual void addNodes(const std::vector<ionet::Node>& nodes, const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) = 0;
		virtual void removeNode(const ProcessId& id) = 0;
		virtual void broadcastNodes(const Payload& payload) = 0;
		virtual void broadcastThisNode() = 0;
		virtual void clearBroadcastData() = 0;
		virtual bool isNodeAdded(const ProcessId& id) = 0;
		virtual void addRemoveNodeResponse(const ProcessId& idToRemove, const ProcessId& respondentId, const Timestamp& timestamp, const Signature& signature) = 0;
		virtual void clearNodeRemovalData() = 0;
		virtual ViewData getUnreachableNodes(ViewData& view) = 0;
	};

	std::shared_ptr<MessageSender> CreateMessageSender(
		ionet::Node thisNode,
		std::weak_ptr<net::PacketWriters> pWriters,
		const ionet::NodeContainer& nodeContainer,
		bool broadcastThisNode,
		std::shared_ptr<TransactionSender> pTransactionSender,
		const dbrb::DbrbViewFetcher& dbrbViewFetcher);
}}