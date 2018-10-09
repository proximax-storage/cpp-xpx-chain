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

#include "BalanceAwareNodeSelector.h"
#include "catapult/cache_core/BalanceView.h"
#include "catapult/extensions/NodeSelector.h"
#include "catapult/ionet/NodeContainer.h"
#include <random>

namespace catapult { namespace timesync {

	namespace {
		struct WeightedCandidatesInfo {
		public:
			WeightedCandidatesInfo() : CumulativeBalance(0)
			{}

		public:
			extensions::WeightedCandidates WeightedCandidates;
			Amount CumulativeBalance;
		};

		template<typename TFilter>
		WeightedCandidatesInfo Filter(const ionet::NodeContainerView& nodeContainerView, TFilter isCandidate) {
			WeightedCandidatesInfo candidatesInfo;
			nodeContainerView.forEach([&candidatesInfo, isCandidate](const auto& node, const auto& nodeInfo) {
				auto pair = isCandidate(node, nodeInfo);
				if (!pair.first)
					return;

				candidatesInfo.WeightedCandidates.emplace_back(node, pair.second.unwrap());
				candidatesInfo.CumulativeBalance = candidatesInfo.CumulativeBalance + pair.second;
			});
			return candidatesInfo;
		}
	}

	BalanceAwareNodeSelector::BalanceAwareNodeSelector(
			ionet::ServiceIdentifier serviceId,
			uint8_t maxNodes,
			Amount minBalance)
			: BalanceAwareNodeSelector(serviceId, maxNodes, minBalance, extensions::SelectCandidatesBasedOnWeight)
	{}

	BalanceAwareNodeSelector::BalanceAwareNodeSelector(
			ionet::ServiceIdentifier serviceId,
			uint8_t maxNodes,
			Amount minBalance,
			const NodeSelector& selector)
			: m_serviceId(serviceId)
			, m_maxNodes(maxNodes)
			, m_minBalance(minBalance)
			, m_selector(selector)
	{}

	ionet::NodeSet BalanceAwareNodeSelector::selectNodes(
			const cache::BalanceView& view,
			const ionet::NodeContainerView& nodeContainerView,
			Height height) const {
		auto candidatesInfo = Filter(nodeContainerView, [this, view, height](const auto& node, const auto& nodeInfo) {
			return this->isCandidate(view, node, nodeInfo, height);
		});

		return m_selector(candidatesInfo.WeightedCandidates, candidatesInfo.CumulativeBalance.unwrap(), m_maxNodes);
	}

	std::pair<bool, Amount> BalanceAwareNodeSelector::isCandidate(
			const cache::BalanceView& view,
			const ionet::Node& node,
			const ionet::NodeInfo& nodeInfo,
			Height height) const {
		auto balance = view.getEffectiveBalance(node.identityKey(), height);
		auto isCandidate = balance >= m_minBalance
				&& !!nodeInfo.getConnectionState(m_serviceId)
				&& ionet::NodeSource::Local != nodeInfo.source();
		return std::make_pair(isCandidate, balance);
	}
}}
