/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "AsyncMessageQueue.h"
#include "SignedNode.h"
#include "catapult/dbrb/DbrbDefinitions.h"

namespace catapult { namespace net { class PacketIoPickerContainer; }}

namespace catapult { namespace dbrb {

	class NodeRetreiver : public AsyncMessageQueue<ProcessId> {
	public:
		explicit NodeRetreiver(const net::PacketIoPickerContainer& packetIoPickers, model::NetworkIdentifier networkIdentifier);
		~NodeRetreiver() override;

	public:
		void enqueue(std::set<ProcessId> ids);
		void addNodes(const std::vector<SignedNode>& nodes);
		std::optional<SignedNode> getNode(const ProcessId& id) const;

	private:
		void processBuffer(BufferType& buffer) override;

	private:
		const net::PacketIoPickerContainer& m_packetIoPickers;
		std::map<ProcessId, SignedNode> m_nodes;
		model::NetworkIdentifier m_networkIdentifier;
	};
}}