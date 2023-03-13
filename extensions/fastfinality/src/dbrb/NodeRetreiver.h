/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SignedNode.h"
#include "catapult/dbrb/DbrbDefinitions.h"
#include <optional>
#include <mutex>

namespace catapult { namespace net { class PacketIoPickerContainer; }}

namespace catapult { namespace dbrb {

	class NodeRetreiver {
	public:
		explicit NodeRetreiver(const net::PacketIoPickerContainer& packetIoPickers, model::NetworkIdentifier networkIdentifier);

	public:
		void requestNodes(const std::set<ProcessId>& requestedIds);
		void addNodes(const std::vector<SignedNode>& nodes);
		void broadcastNodes() const;
		std::optional<SignedNode> getNode(const ProcessId& id) const;

	private:
		const net::PacketIoPickerContainer& m_packetIoPickers;
		std::map<ProcessId, SignedNode> m_nodes;
		model::NetworkIdentifier m_networkIdentifier;
		mutable std::mutex m_mutex;
	};
}}