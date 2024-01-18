/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/dbrb/DbrbDefinitions.h"
#include "catapult/ionet/Node.h"
#include <optional>
#include <mutex>

namespace catapult {
	namespace net {
		class PacketIoPickerContainer;
		class PacketWriters;
	}
	namespace ionet {
		class NodeContainer;
	}
}

namespace catapult { namespace dbrb {

	class NodeRetreiver : public std::enable_shared_from_this<NodeRetreiver> {
	public:
		explicit NodeRetreiver(
			const ProcessId& id,
			std::weak_ptr<net::PacketWriters> pWriters,
			const ionet::NodeContainer& nodeContainer);

	public:
		void requestNodes(const std::set<ProcessId>& requestedIds);
		std::optional<ionet::Node> getNode(const ProcessId& id) const;
		void removeNode(const ProcessId& id);

	private:
		ProcessId m_id;
		std::weak_ptr<net::PacketWriters> m_pWriters;
		const ionet::NodeContainer& m_nodeContainer;
		std::map<ProcessId, ionet::Node> m_nodes;
		mutable std::mutex m_mutex;
	};
}}