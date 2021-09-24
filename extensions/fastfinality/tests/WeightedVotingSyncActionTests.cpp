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

#include <utility>
#include <vector>
#include <fastfinality/src/WeightedVotingChainPackets.h>
#include <catapult/model/Elements.h>
#include <tests/test/core/mocks/MockBlockchainConfigurationHolder.h>
#include <catapult/thread/Future.h>
#include <catapult/thread/IoThreadPool.h>
#include <tests/test/core/ThreadPoolTestUtils.h>
#include <fastfinality/src/WeightedVotingFsm.h>
#include <tests/test/core/mocks/MockCommitteeManager.h>
#include <fastfinality/src/WeightedVotingActions.h>
#include <map>

namespace catapult {

	namespace fastfinality {

#define TEST_CLASS WeightedVotingSyncActionTests

		namespace {

			constexpr uint8_t CommitteeSize = 21u;
			constexpr double CommitteeEndSyncApproval = 0.45;
			constexpr uint64_t CommitteeBaseTotalImportance = 100;
			constexpr double CommitteeNotRunningContribution = 0.5;

			constexpr uint8_t CheckLocalChainActionCode = 0;
			constexpr uint8_t ResetLocalChainActionCode = 1;
			constexpr uint8_t DownloadBlocksActionCode = 2;
			constexpr uint8_t DetectStageActionCode = 3;

			class WeightedVotingSyncActionTestRunner
				: public std::enable_shared_from_this<WeightedVotingSyncActionTestRunner> {
			public:
				WeightedVotingSyncActionTestRunner(
						std::vector<fastfinality::RemoteNodeState> states,
						std::shared_ptr<config::BlockchainConfigurationHolder> configHolder,
						std::shared_ptr<model::BlockElement> lastBlockElement,
						std::map<Key, uint64_t> importance,
						uint8_t correctAction)
					: states(std::move(states))
					, configHolder(configHolder)
					, lastBlockElement(lastBlockElement)
					, importance(importance)
					, correctAction(correctAction)
					, counter(0) {
					pool = test::CreateStartedIoThreadPool();
					fsm = std::make_shared<fastfinality::WeightedVotingFsm>(pool);
				};

				void Start() {
					fsm->start();
					pool->join();
				}

				void AssignActions() {
					auto& actions = fsm->actions();
					actions.CheckLocalChain = [t = shared_from_this()] {
						if (t->counter == 0) {
							t->counter = 1;
							fastfinality::CreateDefaultCheckLocalChainAction(
									t->fsm,
									[t]() -> thread::future<std::vector<RemoteNodeState>> {
										std::vector<RemoteNodeState> states(t->states.size());
										for (int i = 0; i < t->states.size(); i++) {
											states[i] = t->states[i];
										}
										return thread::make_ready_future(std::move(states));
									},
									t->configHolder,
									[t] { return t->lastBlockElement; },
									[t](const Key& key) -> uint64_t { return t->importance[key]; },
									t->committeeManager)();
						} else {
							ASSERT_EQ(t->correctAction, CheckLocalChainActionCode);
						}
					};
					actions.ResetLocalChain = [t = shared_from_this()] {
						ASSERT_EQ(t->correctAction, ResetLocalChainActionCode);
					};
					actions.DownloadBlocks = [t = shared_from_this()] {
						ASSERT_EQ(t->correctAction, DownloadBlocksActionCode);
					};
					actions.DetectStage = [t = shared_from_this()] {
						ASSERT_EQ(t->correctAction, DetectStageActionCode);
					};
				}

			private:
				std::shared_ptr<WeightedVotingFsm> fsm;
				std::vector<fastfinality::RemoteNodeState> states;
				std::shared_ptr<config::BlockchainConfigurationHolder> configHolder;
				std::shared_ptr<const model::BlockElement> lastBlockElement;
				std::map<Key, uint64_t> importance;
				mocks::MockCommitteeManager committeeManager;
				std::shared_ptr<thread::IoThreadPool> pool;
				uint8_t correctAction;
				uint8_t counter;
			};

			auto GenerateStandardConfigHolder() {
				auto config = model::NetworkConfiguration::Uninitialized();
				config.CommitteeEndSyncApproval = CommitteeEndSyncApproval;
				config.CommitteeBaseTotalImportance = CommitteeBaseTotalImportance;
				config.CommitteeNotRunningContribution = CommitteeNotRunningContribution;
				return std::make_shared<config::MockBlockchainConfigurationHolder>(config);
			}
		}

		namespace importance {

			constexpr uint64_t StandardImportance = 1000000u;

			auto GenerateEqualImportance(uint64_t value) {
				std::map<Key, uint64_t> nodeImportance;
				for (int i = 0; i < CommitteeSize; i++) {
					Key key = {{(unsigned char ) i}};
					nodeImportance[key] = value;
				}
				return nodeImportance;
			}
		}

		namespace states {

			constexpr uint64_t MaxHeight = 64u;
			constexpr uint64_t MinHeight = 16u;

			constexpr Hash256 StandardHash = {{1u}};
			constexpr Hash256 NonStandardHash = {{2u}};

			auto GenerateBaseStates() {
				std::vector<fastfinality::RemoteNodeState> states;
				for (int i = 0; i < CommitteeSize; i++) {
					fastfinality::RemoteNodeState r;
					r.NodeKey = {{(unsigned char ) i}};
					r.HarvesterKeys = {r.NodeKey};
					states.push_back(r);
				}
				return states;
			}

			void EqualStates(std::vector<fastfinality::RemoteNodeState> & states, NodeWorkState state,
							 int from, int to) {
				for (int i = from; i < to; i++) {
					states[i].NodeWorkState = state;
				}
			}

			void EqualHashes(std::vector<fastfinality::RemoteNodeState> & states, uint64_t height, Hash256 hash,
							 int from, int to) {
				for (int i = from; i < to; i++) {
					states[i].Height = Height(height);
					states[i].BlockHash = hash;
				}
			}
		}

		namespace blockelement {

			auto GenerateBlockElement(uint64_t height, Hash256 hash) {
				auto b = std::make_shared<model::Block>();
				b->Height = Height(height);
				auto element = std::make_shared<model::BlockElement>(b);
				element->EntityHash = hash;
				return element;
			}

		}

		TEST(TEST_CLASS, GlobalReboot) {
			auto nodeStates = states::GenerateBaseStates();
			states::EqualHashes(nodeStates, states::MaxHeight, states::StandardHash, 0, CommitteeSize);
			states::EqualStates(nodeStates, NodeWorkState::Synchronizing, 0, CommitteeSize);
			auto nodeImportance = importance::GenerateEqualImportance(importance::StandardImportance);
			auto config = GenerateStandardConfigHolder();
			auto element = blockelement::GenerateBlockElement(states::MaxHeight, states::StandardHash);
			auto runner = std::make_shared<WeightedVotingSyncActionTestRunner>(
					nodeStates, config, element, nodeImportance, DetectStageActionCode);
			runner->AssignActions();
			runner->Start();
		}

		TEST(TEST_CLASS, GlobalRebootLagBehind) {
			auto nodeStates = states::GenerateBaseStates();
			states::EqualHashes(nodeStates, states::MaxHeight, states::StandardHash, 0, CommitteeSize);
			states::EqualStates(nodeStates, NodeWorkState::Synchronizing, 0, CommitteeSize);
			auto nodeImportance = importance::GenerateEqualImportance(importance::StandardImportance);
			auto config = GenerateStandardConfigHolder();
			auto element = blockelement::GenerateBlockElement(states::MinHeight, states::NonStandardHash);
			auto runner = std::make_shared<WeightedVotingSyncActionTestRunner>(
					nodeStates, config, element, nodeImportance, DownloadBlocksActionCode);
			runner->AssignActions();
			runner->Start();
		}

		TEST(TEST_CLASS, GlobalRebootNotEnoughApprovalRating) {
			auto nodeStates = states::GenerateBaseStates();
			states::EqualHashes(nodeStates, states::MaxHeight, states::StandardHash, 0, CommitteeSize / 2);
			states::EqualHashes(nodeStates, states::MinHeight, states::NonStandardHash, CommitteeSize / 2, CommitteeSize);
			states::EqualStates(nodeStates, NodeWorkState::Synchronizing, 0, CommitteeSize);
			auto nodeImportance = importance::GenerateEqualImportance(importance::StandardImportance);
			auto config = GenerateStandardConfigHolder();
			auto element = blockelement::GenerateBlockElement(states::MaxHeight, states::NonStandardHash);
			auto runner = std::make_shared<WeightedVotingSyncActionTestRunner>(
					nodeStates, config, element, nodeImportance, CheckLocalChainActionCode);
			runner->AssignActions();
			runner->Start();
		}

		TEST(TEST_CLASS, GlobalRunningEnoughApprovalRating) {
			auto nodeStates = states::GenerateBaseStates();
			states::EqualHashes(nodeStates, states::MaxHeight, states::StandardHash, 0, CommitteeSize / 2);
			states::EqualHashes(nodeStates, states::MinHeight, states::NonStandardHash, CommitteeSize / 2, CommitteeSize);
			states::EqualStates(nodeStates, NodeWorkState::Running, 0, CommitteeSize);
			auto nodeImportance = importance::GenerateEqualImportance(importance::StandardImportance);
			auto config = GenerateStandardConfigHolder();
			auto element = blockelement::GenerateBlockElement(states::MaxHeight, states::NonStandardHash);
			auto runner = std::make_shared<WeightedVotingSyncActionTestRunner>(
					nodeStates, config, element, nodeImportance, DetectStageActionCode);
			runner->AssignActions();
			runner->Start();
		}

		TEST(TEST_CLASS, InvalidChain) {
			auto nodeStates = states::GenerateBaseStates();
			states::EqualHashes(nodeStates, states::MinHeight, states::StandardHash, 0, CommitteeSize);
			states::EqualStates(nodeStates, NodeWorkState::Running, 0, CommitteeSize);
			auto nodeImportance = importance::GenerateEqualImportance(importance::StandardImportance);
			auto config = GenerateStandardConfigHolder();
			auto element = blockelement::GenerateBlockElement(states::MaxHeight, states::NonStandardHash);
			auto runner = std::make_shared<WeightedVotingSyncActionTestRunner>(
					nodeStates, config, element, nodeImportance, ResetLocalChainActionCode);
			runner->AssignActions();
			runner->Start();
		}
	}
}
