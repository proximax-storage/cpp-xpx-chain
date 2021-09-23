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

#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/core/mocks/MockWeightedVotingFsm.h"
#include "extensions/fastfinality/src/WeightedVotingFsm.h"
#include "tests/int/node/test/LocalNodeRequestTestUtils.h"
#include "src/catapult/model/NetworkConfiguration.h"
#include "extensions/fastfinality/src/WeightedVotingChainPackets.h"
#include "tests/test/core/mocks/MockCommitteeManager.h"
#include "extensions/fastfinality/src/WeightedVotingActions.h"
#include <vector>

namespace catapult {

	namespace local {

#define TEST_CLASS H


		constexpr uint8_t Committee_Size = 21u;
		constexpr uint64_t ConstantImportance = 1000000u;
		constexpr uint64_t MaxHeight = 64u;

		class A {
		public:
			A() {

			}
		private:
			int counter;
		};

		class Temp {
		private:

		};
		int z = 0;

		void RunCreateDefaultCheckLocalChainActionTest(int correct_action,
			    std::shared_ptr<catapult::fastfinality::RemoteNodeStateRetriever> retriever,
				std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder,
				std::shared_ptr<model::BlockElementSupplier> lastBlockElementSupplier,
				std::shared_ptr<std::function<uint64_t (const Key&)>> importanceGetter) {
			std::shared_ptr<thread::IoThreadPool> pPool = test::CreateStartedIoThreadPool();
			auto fsmShared = std::make_shared<fastfinality::WeightedVotingFsm>(pPool);
			// auto fsmWeak = std::weak_ptr<fastfinality::WeightedVotingFsm>(fsmShared);
			auto committeeManager = std::make_shared<mocks::MockCommitteeManager>();
			auto counter = std::make_shared<int>(0);
			auto & actions = fsmShared->actions();
			actions.CheckLocalChain = [correct_action, fsmShared,
									   retriever, pConfigHolder, lastBlockElementSupplier,
									   importanceGetter, committeeManager] () {
				if (z == 0) {
					z += 1;
					auto weakFsm = std::weak_ptr<fastfinality::WeightedVotingFsm>(fsmShared);
					fastfinality::CreateDefaultCheckLocalChainAction(
							weakFsm, *retriever, pConfigHolder,
							*lastBlockElementSupplier, *importanceGetter, *committeeManager)();
				}	else {
					ASSERT_EQ(correct_action, 0);
				}
			};
			actions.ResetLocalChain = [correct_action] {
				ASSERT_EQ(correct_action, 1);
			};
			actions.DownloadBlocks = [correct_action] {
				ASSERT_EQ(correct_action, 2);
			};
			actions.DetectStage = [correct_action] {
				ASSERT_EQ(correct_action, 3);
			};
			fsmShared->start();
		}





		uint64_t ConstantImportanceGenerator(const Key& key) {
			return ConstantImportance;
		}
//		struct RemoteNodeState {
//			catapult::Height Height;
//			Hash256 BlockHash;
//			fastfinality::NodeWorkState NodeWorkState;
//			Key NodeKey;
//			std::vector<Key> HarvesterKeys;
//		};

		auto GenerateStates() {
			std::vector<fastfinality::RemoteNodeState>states;
			for (int i = 0; i < Committee_Size; i++) {
				fastfinality::RemoteNodeState r;
				r.Height = Height(MaxHeight);
				r.BlockHash = {{1u}};
				r.NodeKey = {{(unsigned char ) i}};
				r.NodeWorkState = fastfinality::NodeWorkState::Synchronizing;
				r.HarvesterKeys = {r.NodeKey};
				states.push_back(r);
			}
			return states;
		}

		auto GenerateBlockElement() {
			model::Block b;
			b.Height = Height(MaxHeight);
			auto element = new model::BlockElement(b);
			element->EntityHash = {{1u}};
			return std::shared_ptr<model::BlockElement>(element);
		}

		auto CreateConfigHolder() {
			auto config = model::NetworkConfiguration::Uninitialized();
			config.CommitteeEndSyncApproval = 0.45;
			config.CommitteeBaseTotalImportance = 100;
			config.CommitteeNotRunningContribution = 0.5;
			return std::make_shared<config::MockBlockchainConfigurationHolder>(config);
		}

		auto Retriever() {
			return thread::make_ready_future(GenerateStates());
		}

		auto Supplier() {
			return GenerateBlockElement();
		}

		void RunTemp() {
			std::shared_ptr<thread::IoThreadPool> pPool = test::CreateStartedIoThreadPool();
			auto fsmShared = std::make_shared<fastfinality::WeightedVotingFsm>(pPool);
			auto configHolder = CreateConfigHolder();
			auto committeeManager = std::make_shared<mocks::MockCommitteeManager>();
			auto & actions = fsmShared->actions();
			actions.CheckLocalChain = [fsmShared, configHolder, committeeManager] () {
				auto weak = std::weak_ptr<fastfinality::WeightedVotingFsm>(fsmShared);
				fastfinality::CreateDefaultCheckLocalChainAction(
						weak,
						Retriever,
						configHolder,
						Supplier,
						ConstantImportanceGenerator,
						*committeeManager)();
			};
			fsmShared->start();
		}

		TEST(TEST_CLASS, TEST_NAME) {
//			auto configHolder = CreateConfigHolder();
////			catapult::fastfinality::RemoteNodeStateRetriever statesRetriever = [] () {};
//			auto s = 6;
//			// std::function<int(void)> a = [] () {return 3;};
//			auto supplier = [] () {return GenerateBlockElement();};
//			auto committeeManager = catapult::mocks::MockCommitteeManager();
			RunTemp();
//			RunTemp(std::make_shared<catapult::fastfinality::RemoteNodeStateRetriever>(statesRetriever), configHolder, std::make_shared<model::BlockElementSupplier>(supplier), std::make_shared<std::function<uint64_t (const Key&)>>(ConstantImportanceGenerator));
		}
	}
//
	// Created by kyrylo on 22.09.2021.
	//
}