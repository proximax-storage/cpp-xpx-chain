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

#include "timesync/src/ImportanceAwareNodeSelector.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/ImportanceView.h"
#include "catapult/ionet/NodeContainer.h"
#include "catapult/ionet/NodeInfo.h"
#include "catapult/utils/ArraySet.h"
#include "tests/test/cache/ImportanceViewTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/net/NodeTestUtils.h"
#include "tests/test/nodeps/Waits.h"
#include "tests/test/nodeps/TestConstants.h"
#include "tests/test/other/NodeSelectorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace timesync {

#define TEST_CLASS ImportanceAwareNodeSelectorTests

	namespace {
		constexpr ionet::ServiceIdentifier Default_Service_Identifier(123);
		constexpr model::ImportanceHeight Default_Importance_Height(234);

		void SeedAccountStateCache(
				cache::AccountStateCache& cache,
				const std::vector<Key>& keys,
				const std::vector<Importance>& importances) {
			auto delta = cache.createDelta(Height{0});
			for (auto i = 0u; i < keys.size(); ++i) {
				delta->addAccount(keys[i], Height(1));
				auto& accountState = delta->find(keys[i]).get();
				accountState.Balances.track(test::Default_Harvesting_Mosaic_Id);
				accountState.Balances.credit(test::Default_Harvesting_Mosaic_Id, Amount(importances[i].unwrap()), Height(1));
			}

			cache.commit();
		}

		auto CreateAccountStateCache() {
			auto cacheConfig = cache::CacheConfiguration();
			auto maxAmount = Amount(std::numeric_limits<Amount::ValueType>::max());
			auto networkConfig = model::NetworkConfiguration::Uninitialized();
			networkConfig.Info.Identifier = model::NetworkIdentifier::Mijin_Test;
			networkConfig.ImportanceGrouping = 234;
			networkConfig.MinHarvesterBalance = maxAmount;
			networkConfig.CurrencyMosaicId = MosaicId(1111);
			networkConfig.HarvestingMosaicId = MosaicId(2222);
			auto pConfigHolder = config::CreateMockConfigurationHolder(networkConfig);
			return std::make_unique<cache::AccountStateCache>(cacheConfig, pConfigHolder);
		}

		struct SeedNodeContainerOptions {
		public:
			SeedNodeContainerOptions()
					: IsActive(true)
					, NodeSource(ionet::NodeSource::Dynamic)
			{}

		public:
			bool IsActive;
			ionet::NodeSource NodeSource;
		};

		void SeedNodeContainer(
				ionet::NodeContainer& nodeContainer,
				const std::vector<Key>& keys,
				const SeedNodeContainerOptions& options = SeedNodeContainerOptions()) {
			auto i = 0u;
			auto modifier = nodeContainer.modifier();
			for (const auto& key : keys) {
				auto nodeName = "Node" + std::to_string(++i);

				modifier.add(test::CreateNamedNode(key, nodeName, ionet::NodeRoles::Peer), options.NodeSource);
				if (options.IsActive)
					modifier.provisionConnectionState(Default_Service_Identifier, key).Age = 5;
			}
		}
	}

	// region no nodes selected

	namespace {
		struct Options {
		public:
			Options()
					: NodeServiceIdentifier(Default_Service_Identifier)
					, NodeImportance(1234)
					, NodeImportanceHeight(Default_Importance_Height)
			{}

		public:
			ionet::ServiceIdentifier NodeServiceIdentifier;
			Importance NodeImportance;
			model::ImportanceHeight NodeImportanceHeight;
			SeedNodeContainerOptions SeedNodeOptions;
		};

		void AssertNoNodesAreSelected(const Options& options) {
			// Arrange:
			auto keys = test::GenerateRandomDataVector<Key>(3);
			std::vector<Importance> importances(3, options.NodeImportance);
			auto pCache = CreateAccountStateCache();
			SeedAccountStateCache(*pCache, keys, importances);

			ionet::NodeContainer nodeContainer;
			SeedNodeContainer(nodeContainer, keys, options.SeedNodeOptions);
			auto pView = test::CreateImportanceView(*pCache);
			ImportanceAwareNodeSelector selector(options.NodeServiceIdentifier, 5, Importance(1000));

			// Act:
			auto selectNodes = selector.selectNodes(*pView, nodeContainer.view(), Height(100));

			// Assert:
			EXPECT_TRUE(selectNodes.empty());
		}
	}

	TEST(TEST_CLASS, ReturnsEmptySetIfNoNodeHasEnoughImportance) {
		// Arrange:
		Options options;
		options.NodeImportance = Importance(123);

		// Assert:
		AssertNoNodesAreSelected(options);
	}

	TEST(TEST_CLASS, ReturnsEmptySetIfNoNodeHasAnActiveConnectionStateForAnyService) {
		Options options;
		options.SeedNodeOptions.IsActive = false;

		// Assert:
		AssertNoNodesAreSelected(options);
	}

	TEST(TEST_CLASS, ReturnsEmptySetIfNodeHasActiveConnectionStateOnlyForNonMatchingService) {
		Options options;
		options.NodeServiceIdentifier = ionet::ServiceIdentifier(321);

		// Assert:
		AssertNoNodesAreSelected(options);
	}

	TEST(TEST_CLASS, ReturnsEmptySetIfNodeIsLocal) {
		Options options;
		options.SeedNodeOptions.NodeSource = ionet::NodeSource::Local;

		// Assert:
		AssertNoNodesAreSelected(options);
	}

	// endregion

	// region node are selected

	namespace {
		template<typename TAssert>
		void AssertSelectedNodes(const std::vector<Key>& keys, const std::vector<Importance>& importances, TAssert assertKeys) {
			// Arrange:
			auto pCache = CreateAccountStateCache();
			SeedAccountStateCache(*pCache, keys, importances);

			ionet::NodeContainer nodeContainer;
			SeedNodeContainer(nodeContainer, keys);
			auto pView = test::CreateImportanceView(*pCache);
			ImportanceAwareNodeSelector selector(Default_Service_Identifier, 3, Importance(1000));

			// Act:
			auto selectNodes = selector.selectNodes(*pView, nodeContainer.view(), Height(1));

			// Assert:
			assertKeys(test::ExtractNodeIdentities(selectNodes));
		}
	}

	TEST(TEST_CLASS, ReturnsOnlyNodesThatMeetAllRequirements) {
		// Arrange: only importances at indexes 0, 2 and 3 qualify
		std::vector<Importance> importances{ Importance(1234), Importance(123), Importance(5000), Importance(10000), Importance(50) };
		auto allKeys = test::GenerateRandomDataVector<Key>(5);
		utils::KeySet expectedKeys{ allKeys[0], allKeys[2], allKeys[3] };

		// Act:
		AssertSelectedNodes(allKeys, importances, [&expectedKeys](const auto& keys) {
			// Assert:
			EXPECT_EQ(3u, keys.size());
			EXPECT_EQ(expectedKeys, keys);
		});
	}

	// endregion

	// region delegation

	namespace {
		struct SelectorParamCapture {
			extensions::WeightedCandidates WeightedCandidates;
			uint64_t TotalWeight;
			size_t MaxCandidates;
			size_t NumSelectorCalls = 0;
		};
	}

	TEST(TEST_CLASS, UsesProvidedCustomSelector) {
		// Arrange:
		std::vector<Importance> importances(5, Importance(1000));
		auto keys = test::GenerateRandomDataVector<Key>(importances.size());
		auto pCache = CreateAccountStateCache();
		SeedAccountStateCache(*pCache, keys, importances);

		ionet::NodeContainer nodeContainer;
		SeedNodeContainer(nodeContainer, keys);

		auto pView = test::CreateImportanceView(*pCache);
		SelectorParamCapture capture;
		auto customSelector = [&capture](const auto& weightedCandidates, auto totalWeight, auto maxCandidates) {
			for (const auto& weightedCandidate : weightedCandidates)
				capture.WeightedCandidates.push_back(weightedCandidate);

			capture.TotalWeight = totalWeight;
			capture.MaxCandidates = maxCandidates;
			++capture.NumSelectorCalls;
			return ionet::NodeSet();
		};
		ImportanceAwareNodeSelector selector(Default_Service_Identifier, 3, Importance(1000), customSelector);

		// Act:
		selector.selectNodes(*pView, nodeContainer.view(), Height(Default_Importance_Height.unwrap() + 1));

		// Assert:
		EXPECT_EQ(1u, capture.NumSelectorCalls);
		EXPECT_EQ(nodeContainer.view().size(), capture.WeightedCandidates.size());
		std::unordered_set<Key, utils::ArrayHasher<Key>> keySet(keys.cbegin(), keys.cend());
		for (const auto& candidate : capture.WeightedCandidates)
			EXPECT_CONTAINS(keySet, candidate.Node.identityKey());

		EXPECT_EQ(5000u, capture.TotalWeight);
		EXPECT_EQ(3u, capture.MaxCandidates);
	}

	// endregion

	// region probability test

	namespace {
		struct ImportanceAwareNodeSelectorTraits {
		public:
			using KeyStatistics = std::unordered_map<Key, uint32_t, utils::ArrayHasher<Key>>;
			static constexpr auto Description = "node selection and importance correlation";

			static KeyStatistics CreateStatistics(
					const std::vector<ionet::Node>& nodes,
					const std::vector<uint64_t>& rawWeights,
					uint64_t numIterations) {
				uint64_t cummulativeWeight = 0u;
				std::vector<Importance> importances;
				for (auto weight : rawWeights) {
					cummulativeWeight += weight;
					importances.push_back(Importance(weight));
				}

				std::vector<Key> keys;
				for (const auto& node : nodes)
					keys.push_back(node.identityKey());

				auto pCache = CreateAccountStateCache();
				SeedAccountStateCache(*pCache, keys, importances);

				ionet::NodeContainer nodeContainer;
				SeedNodeContainer(nodeContainer, keys);
				auto pView = test::CreateImportanceView(*pCache);
				ImportanceAwareNodeSelector selector(Default_Service_Identifier, 1, Importance(1000));

				KeyStatistics keyStatistics;
				for (auto i = 0u; i < numIterations; ++i) {
					auto selectedNodes = selector.selectNodes(*pView, nodeContainer.view(), Height(1));
					if (1u != selectedNodes.size())
						CATAPULT_THROW_RUNTIME_ERROR_1("unexpected number of nodes were selected", selectedNodes.size());

					++keyStatistics[selectedNodes.cbegin()->identityKey()];
				}

				return keyStatistics;
			}
		};
	}

	DEFINE_NODE_SELECTOR_PROBABILITY_TESTS(ImportanceAwareNodeSelector)

	// endregion
}}
