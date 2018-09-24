/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include "catapult/extensions/NodeSelector.h"
#include "catapult/ionet/Node.h"
#include "catapult/ionet/NodeInfo.h"

namespace catapult {
	namespace cache { class BalanceView; }
	namespace ionet { class NodeContainerView; }
}

namespace catapult { namespace timesync {

	/// A node selector that selects nodes according to the importance of the account used to boot the node.
	class BalanceAwareNodeSelector {
	public:
		using NodeSelector = std::function<ionet::NodeSet (extensions::WeightedCandidates&, uint64_t, size_t)>;

	public:
		/// Creates a selector that can pick up to \a maxNodes nodes with a minimum balance of \a minBalance
		/// that have active connections with service id \a serviceId.
		explicit BalanceAwareNodeSelector(ionet::ServiceIdentifier serviceId, uint8_t maxNodes, Amount minBalance);

		/// Creates a selector around a custom \a selector that can pick up to \a maxNodes nodes with
		/// a minimum balance of \a minBalance that have active connections with service id \a serviceId.
		explicit BalanceAwareNodeSelector(
				ionet::ServiceIdentifier serviceId,
				uint8_t maxNodes,
				Amount minBalance,
				const NodeSelector& selector);

	public:
		/// Selects nodes from \a nodeContainerView that have a minimum importance at \a height according to \a importanceView.
		ionet::NodeSet selectNodes(
				const cache::BalanceView& view,
				const ionet::NodeContainerView& nodeContainerView,
				Height height) const;

	private:
		std::pair<bool, Amount> isCandidate(
				const cache::BalanceView& view,
				const ionet::Node& node,
				const ionet::NodeInfo& nodeInfo,
				Height height) const;

	private:
		ionet::ServiceIdentifier m_serviceId;
		uint8_t m_maxNodes;
		Amount m_minBalance;
		NodeSelector m_selector;
	};
}}
