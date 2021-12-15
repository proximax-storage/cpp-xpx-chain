/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "fastfinality/src/WeightedVotingFsm.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/core/mocks/MockCommitteeManager.h"
#include "tests/test/core/ThreadPoolTestUtils.h"

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

			constexpr uint64_t StandardImportance = 1000000u;

			constexpr uint64_t MaxHeight = 64u;
			constexpr uint64_t MinHeight = 16u;

			constexpr Hash256 StandardHash = {{1u}};
			constexpr Hash256 NonStandardHash = {{2u}};

			class WeightedVotingSyncActionTestRunner : public std::enable_shared_from_this<WeightedVotingSyncActionTestRunner> {
			public:
				WeightedVotingSyncActionTestRunner(
						std::vector<fastfinality::RemoteNodeState> states,
						std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder,
						std::shared_ptr<model::BlockElement> pLastBlockElement,
						std::map<Key, uint64_t> importances,
						uint8_t expectedAction)
					: m_pPool(test::CreateStartedIoThreadPool())
					, m_pFsm(std::make_shared<fastfinality::WeightedVotingFsm>(m_pPool, pConfigHolder->Config()))
					, m_states(std::move(states))
					, m_pConfigHolder(pConfigHolder)
					, m_pLastBlockElement(pLastBlockElement)
					, m_importances(std::move(importances))
					, m_committeeManager()
					, m_expectedAction(expectedAction)
					, m_counter(0)
				{};

				void start() {
					m_pFsm->start();
					m_pPool->join();
				}

				void assignActions() {
					auto& actions = m_pFsm->actions();
					actions.CheckLocalChain = [pThis = shared_from_this()] {
						if (pThis->m_counter == 0) {
							pThis->m_counter++;
							auto defaultCheckLocalChainAction = fastfinality::CreateDefaultCheckLocalChainAction(
								pThis->m_pFsm,
									[pThis]() -> thread::future<std::vector<RemoteNodeState>> {
										auto states = pThis->m_states;
										return thread::make_ready_future(std::move(states));
									},
								pThis->m_pConfigHolder,
									[pThis] { return pThis->m_pLastBlockElement; },
									[pThis](const Key& key) -> uint64_t { return pThis->m_importances[key]; },
								pThis->m_committeeManager);

							defaultCheckLocalChainAction();
						} else {
							ASSERT_EQ(pThis->m_expectedAction, CheckLocalChainActionCode);
						}
					};
					actions.ResetLocalChain = [pThis = shared_from_this()] {
						ASSERT_EQ(pThis->m_expectedAction, ResetLocalChainActionCode);
					};
					actions.DownloadBlocks = [pThis = shared_from_this()] {
						ASSERT_EQ(pThis->m_expectedAction, DownloadBlocksActionCode);
					};
					actions.DetectStage = [pThis = shared_from_this()] {
						ASSERT_EQ(pThis->m_expectedAction, DetectStageActionCode);
					};
				}

			private:
				std::shared_ptr<thread::IoThreadPool> m_pPool;
				std::shared_ptr<WeightedVotingFsm> m_pFsm;
				std::vector<fastfinality::RemoteNodeState> m_states;
				std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
				std::shared_ptr<const model::BlockElement> m_pLastBlockElement;
				std::map<Key, uint64_t> m_importances;
				mocks::MockCommitteeManager m_committeeManager;
				uint8_t m_expectedAction;
				uint8_t m_counter;
			};

			auto CreateConfigHolder() {
				auto config = model::NetworkConfiguration::Uninitialized();
				config.CommitteeEndSyncApproval = CommitteeEndSyncApproval;
				config.CommitteeBaseTotalImportance = CommitteeBaseTotalImportance;
				config.CommitteeNotRunningContribution = CommitteeNotRunningContribution;
				return std::make_shared<config::MockBlockchainConfigurationHolder>(config);
			}
		}

		auto GenerateEqualImportances(uint64_t value) {
			std::map<Key, uint64_t> nodeImportances;
			for (int i = 0; i < CommitteeSize; i++) {
				Key key = {{(unsigned char ) i}};
				nodeImportances[key] = value;
			}
			return nodeImportances;
		}

		auto GenerateBaseStates() {
			std::vector<fastfinality::RemoteNodeState> states;
			for (int i = 0; i < CommitteeSize; i++) {
				fastfinality::RemoteNodeState state;
				state.NodeKey = {{(unsigned char ) i}};
				state.HarvesterKeys = {state.NodeKey};
				states.push_back(state);
			}
			return states;
		}

		void EqualStates(std::vector<fastfinality::RemoteNodeState>& states, NodeWorkState state, int from, int to) {
			for (int i = from; i < to; i++)
				states[i].NodeWorkState = state;
		}

		void EqualHashes(std::vector<fastfinality::RemoteNodeState>& states, uint64_t height, const Hash256& hash, int from, int to) {
			for (int i = from; i < to; i++) {
				states[i].Height = Height(height);
				states[i].BlockHash = hash;
			}
		}

		auto GenerateBlockElement(uint64_t height, const Hash256& hash) {
			auto pBlock = std::make_shared<model::Block>();
			pBlock->Height = Height(height);
			auto pBlockElement = std::make_shared<model::BlockElement>(pBlock);
			pBlockElement->EntityHash = hash;
			return pBlockElement;
		}

		TEST(TEST_CLASS, GlobalReboot) {
			auto nodeStates = GenerateBaseStates();
			EqualHashes(nodeStates, MaxHeight, StandardHash, 0, CommitteeSize);
			EqualStates(nodeStates, NodeWorkState::Synchronizing, 0, CommitteeSize);
			auto pRunner = std::make_shared<WeightedVotingSyncActionTestRunner>(
				nodeStates, CreateConfigHolder(), GenerateBlockElement(MaxHeight, StandardHash),
				GenerateEqualImportances(StandardImportance), DetectStageActionCode);
			pRunner->assignActions();
			pRunner->start();
		}

		TEST(TEST_CLASS, GlobalRebootLagBehind) {
			auto nodeStates = GenerateBaseStates();
			EqualHashes(nodeStates, MaxHeight, StandardHash, 0, CommitteeSize);
			EqualStates(nodeStates, NodeWorkState::Synchronizing, 0, CommitteeSize);
			auto pRunner = std::make_shared<WeightedVotingSyncActionTestRunner>(
				nodeStates, CreateConfigHolder(), GenerateBlockElement(MinHeight, NonStandardHash),
				GenerateEqualImportances(StandardImportance), DownloadBlocksActionCode);
			pRunner->assignActions();
			pRunner->start();
		}

		TEST(TEST_CLASS, GlobalRebootNotEnoughApprovalRating) {
			auto nodeStates = GenerateBaseStates();
			EqualHashes(nodeStates, MaxHeight, StandardHash, 0, CommitteeSize / 2);
			EqualHashes(nodeStates, MinHeight, NonStandardHash, CommitteeSize / 2, CommitteeSize);
			EqualStates(nodeStates, NodeWorkState::Synchronizing, 0, CommitteeSize);
			auto pRunner = std::make_shared<WeightedVotingSyncActionTestRunner>(
				nodeStates, CreateConfigHolder(), GenerateBlockElement(MaxHeight, NonStandardHash),
				GenerateEqualImportances(StandardImportance), CheckLocalChainActionCode);
			pRunner->assignActions();
			pRunner->start();
		}

		TEST(TEST_CLASS, GlobalRunningEnoughApprovalRating) {
			auto nodeStates = GenerateBaseStates();
			EqualHashes(nodeStates, MaxHeight, StandardHash, 0, CommitteeSize / 2);
			EqualHashes(nodeStates, MinHeight, NonStandardHash, CommitteeSize / 2, CommitteeSize);
			EqualStates(nodeStates, NodeWorkState::Running, 0, CommitteeSize);
			auto pRunner = std::make_shared<WeightedVotingSyncActionTestRunner>(
				nodeStates, CreateConfigHolder(), GenerateBlockElement(MaxHeight, NonStandardHash),
				GenerateEqualImportances(StandardImportance), DetectStageActionCode);
			pRunner->assignActions();
			pRunner->start();
		}

		TEST(TEST_CLASS, InvalidChain) {
			auto nodeStates = GenerateBaseStates();
			EqualHashes(nodeStates, MinHeight, StandardHash, 0, CommitteeSize);
			EqualStates(nodeStates, NodeWorkState::Running, 0, CommitteeSize);
			auto pRunner = std::make_shared<WeightedVotingSyncActionTestRunner>(
				nodeStates, CreateConfigHolder(), GenerateBlockElement(MaxHeight, NonStandardHash),
				GenerateEqualImportances(StandardImportance), ResetLocalChainActionCode);
			pRunner->assignActions();
			pRunner->start();
		}
	}
}
