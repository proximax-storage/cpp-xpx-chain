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

#include "ImportanceAwareNodeSelector.h"
#include "catapult/cache_core/ImportanceView.h"
#include "catapult/extensions/NodeSelector.h"
#include "catapult/ionet/NodeContainer.h"
#include <random>
#include <utility>

namespace catapult { namespace timesync {

	namespace {
		struct WeightedCandidatesInfo {
		public:
			WeightedCandidatesInfo() : CumulativeImportance(0)
			{}

		public:
			extensions::WeightedCandidates WeightedCandidates;
			Importance CumulativeImportance;
		};

		template<typename TFilter>
		WeightedCandidatesInfo Filter(const ionet::NodeContainerView& nodeContainerView, TFilter isCandidate) {
			WeightedCandidatesInfo candidatesInfo;
			nodeContainerView.forEach([&candidatesInfo, isCandidate](const auto& node, const auto& nodeInfo) {
				auto pair = isCandidate(node, nodeInfo);
				if (!pair.first)
					return;

				candidatesInfo.WeightedCandidates.emplace_back(node, pair.second.unwrap());
				candidatesInfo.CumulativeImportance = candidatesInfo.CumulativeImportance + pair.second;
			});
			return candidatesInfo;
		}
	}

	ImportanceAwareNodeSelector::ImportanceAwareNodeSelector(
			ionet::ServiceIdentifier serviceId,
			uint8_t maxNodes,
			Importance minImportance)
			: ImportanceAwareNodeSelector(std::move(serviceId), maxNodes, std::move(minImportance), extensions::SelectCandidatesBasedOnWeight)
	{}

	ImportanceAwareNodeSelector::ImportanceAwareNodeSelector(
			ionet::ServiceIdentifier serviceId,
			uint8_t maxNodes,
			Importance minImportance,
			NodeSelector selector)
			: m_serviceId(std::move(serviceId))
			, m_maxNodes(maxNodes)
			, m_minImportance(std::move(minImportance))
			, m_selector(std::move(selector))
	{}

	ionet::NodeSet ImportanceAwareNodeSelector::selectNodes(
			const cache::ImportanceView& importanceView,
			const ionet::NodeContainerView& nodeContainerView,
			const Height& height,
			bool enableProperCalculation) const {
		auto candidatesInfo = Filter(nodeContainerView, [this, &importanceView, height, enableProperCalculation](const auto& node, const auto& nodeInfo) {
			return this->isCandidate(importanceView, node, nodeInfo, height, enableProperCalculation);
		});

		return m_selector(candidatesInfo.WeightedCandidates, candidatesInfo.CumulativeImportance.unwrap(), m_maxNodes);
	}

	std::pair<bool, Importance> ImportanceAwareNodeSelector::isCandidate(
			const cache::ImportanceView& importanceView,
			const ionet::Node& node,
			const ionet::NodeInfo& nodeInfo,
			Height height,
			bool enableProperCalculation) const {
		auto importance = importanceView.getAccountImportanceOrDefault(node.identityKey(), std::move(height), enableProperCalculation);
		auto isCandidate = importance >= m_minImportance
				&& !!nodeInfo.getConnectionState(m_serviceId)
				&& ionet::NodeSource::Local != nodeInfo.source();
		return std::make_pair(isCandidate, importance);
	}
}}
