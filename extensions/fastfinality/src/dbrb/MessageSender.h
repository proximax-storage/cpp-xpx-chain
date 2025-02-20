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
	namespace net { class PacketReadersWriters; }
	namespace dbrb { class TransactionSender; }
	namespace config { class BlockchainConfigurationHolder; }
	namespace thread { class IoThreadPool; }
	namespace utils { class TimeSpan; }
}

namespace catapult { namespace dbrb {

	class MessageSender {
	public:
		virtual ~MessageSender() = default;

	public:
		// Message sending
		virtual void enqueue(const Payload& payload, bool dropOnFailure, const std::set<ProcessId>& recipients) = 0;
		virtual void clearQueue() = 0;

	public:
		// Node discovery
		virtual void connectNodes(const std::set<ProcessId>& requestedIds) = 0;
		virtual void closeAllConnections() = 0;
		virtual void closeConnections(const std::set<ProcessId>& requestedIds) = 0;
		virtual void addNodes(const std::vector<ionet::Node>& nodes) = 0;
		virtual void sendNodes(const std::vector<ionet::Node>& nodes, const ProcessId& recipient) = 0;
		virtual ViewData getUnreachableNodes(ViewData& view) const = 0;
		virtual size_t getUnreachableNodeCount(const dbrb::ViewData& view) const = 0;
		virtual std::vector<ionet::Node> getKnownNodes(const ViewData& view) const = 0;

	public:
		void setWriters(std::weak_ptr<net::PacketReadersWriters> pWriters) {
			m_pWriters = std::move(pWriters);
		}

	protected:
		std::weak_ptr<net::PacketReadersWriters> m_pWriters;
	};

	std::shared_ptr<MessageSender> CreateMessageSender(
		ionet::Node thisNode,
		const ionet::NodeContainer& nodeContainer,
		bool broadcastThisNode,
		const std::shared_ptr<thread::IoThreadPool>& pPool,
		const utils::TimeSpan& resendMessagesInterval);
}}