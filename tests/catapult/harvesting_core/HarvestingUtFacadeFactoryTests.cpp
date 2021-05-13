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

#include "catapult/harvesting_core/HarvestingUtFacadeFactory.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/model/BlockUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/core/TransactionInfoTestUtils.h"
#include "tests/test/nodeps/Filesystem.h"
#include "tests/test/other/MockExecutionConfiguration.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"

namespace catapult { namespace harvesting {

#define TEST_CLASS HarvestingUtFacadeFactoryTests

	namespace {
		// region constants

		constexpr auto Default_Height = Height(17);
		constexpr auto Default_Time = Timestamp(987);

		constexpr auto Currency_Mosaic_Id = MosaicId(1234);
		constexpr auto Harvesting_Mosaic_Id = MosaicId(9876);

		// endregion

		// region CreateConfiguration

		enum class StateVerifyOptions { None = 0, State = 1, Receipts = 2, All = 3 };

		constexpr bool HasFlag(StateVerifyOptions testedFlag, StateVerifyOptions value) {
			return utils::to_underlying_type(testedFlag) == (utils::to_underlying_type(testedFlag) & utils::to_underlying_type(value));
		}

		auto CreateConfiguration(StateVerifyOptions verifyOptions = StateVerifyOptions::None) {
			test::MutableBlockchainConfiguration config;

			config.Immutable.NetworkIdentifier = model::NetworkIdentifier::Mijin_Test;
			config.Immutable.ShouldEnableVerifiableState = HasFlag(StateVerifyOptions::State, verifyOptions);
			config.Immutable.ShouldEnableVerifiableReceipts = HasFlag(StateVerifyOptions::Receipts, verifyOptions);
			config.Immutable.CurrencyMosaicId = Currency_Mosaic_Id;
			config.Immutable.HarvestingMosaicId = Harvesting_Mosaic_Id;

			config.Network.ImportanceGrouping = 4;
			config.Network.MaxTransactionLifetime = utils::TimeSpan::FromHours(24);
			config.Network.MinHarvesterBalance = Amount(1000);
			config.Network.BlockPruneInterval = 10;
			config.Network.GreedDelta = 0.5;
			config.Network.GreedExponent = 2.0;

			config.Node.FeeInterest = 1;
			config.Node.FeeInterestDenominator = 1;

			return config.ToConst();
		}

		// endregion

		// region RunUtFacadeTest / AssertEmpty

		template<typename TAction>
		void RunUtFacadeTest(TAction action) {
			// Arrange: create factory and facade
			auto catapultCache = test::CreateCatapultCacheWithMarkerAccount(Default_Height);
			test::MockExecutionConfiguration executionConfig(CreateConfiguration());
			HarvestingUtFacadeFactory factory(catapultCache, executionConfig.Config);

			auto pFacade = factory.create(Default_Time);
			ASSERT_TRUE(!!pFacade);

			// Act + Assert:
			action(*pFacade, executionConfig);
		}

		template<typename TAction>
		void RunUtFacadeTest(uint32_t numTransactionInfos, TAction action) {
			// Arrange: create transactions with zero max fees (this causes none to have a surplus when block fee multiplier is zero)
			std::vector<std::pair<uint32_t, uint32_t>> sizeMultiplierPairs;
			for (auto i = 0u; i < numTransactionInfos; ++i)
				sizeMultiplierPairs.emplace_back(200, 0);

			auto transactionInfos = test::CreateTransactionInfosFromSizeMultiplierPairs(sizeMultiplierPairs);

			// - create factory and facade
			auto catapultCache = test::CreateCatapultCacheWithMarkerAccount(Default_Height);
			test::MockExecutionConfiguration executionConfig(CreateConfiguration());
			HarvestingUtFacadeFactory factory(catapultCache, executionConfig.Config);

			auto pFacade = factory.create(Default_Time);
			ASSERT_TRUE(!!pFacade);

			// Act + Assert:
			action(*pFacade, transactionInfos, executionConfig);
		}

		void AssertEmpty(const HarvestingUtFacade& facade) {
			// Assert:
			EXPECT_EQ(Default_Height + Height(1), facade.height());
			EXPECT_EQ(0u, facade.size());
			EXPECT_EQ(0u, facade.transactionInfos().size());
		}

		// endregion

		// region entity / context asserts

		template<typename TParams>
		void AssertEntityInfos(
				const std::string& tag,
				const TParams& params,
				const std::vector<Hash256>& transactionHashes,
				const std::vector<std::pair<size_t, size_t>>& expectedIndexIdPairs) {
			// Assert:
			ASSERT_EQ(expectedIndexIdPairs.size(), params.size());

			for (auto i = 0u; i < params.size(); ++i) {
				const auto& pair = expectedIndexIdPairs[i];
				EXPECT_EQ(transactionHashes[pair.first], params[i].HashCopy) << tag << " param at " << i;
				EXPECT_EQ(pair.second, params[i].SequenceId) << tag << " param at " << i;
			}
		}

		void AssertValidatorContexts(
				const test::MockExecutionConfiguration& executionConfig,
				const std::vector<size_t>& expectedNumDifficultyInfos) {
			// Assert:
			test::MockExecutionConfiguration::AssertValidatorContexts(
					*executionConfig.pValidator,
					expectedNumDifficultyInfos,
					Default_Height + Height(1),
					Default_Time);
		}

		void AssertObserverContexts(
				const test::MockExecutionConfiguration& executionConfig,
				size_t numObserverCalls) {
			// Assert:
			EXPECT_EQ(numObserverCalls, executionConfig.pObserver->params().size());
			test::MockExecutionConfiguration::AssertObserverContexts(
					*executionConfig.pObserver,
					0,
					Default_Height + Height(1),
					model::ImportanceHeight(16),
					[](auto) { return false; });
		}

		// endregion
	}

	// region constructor

	TEST(TEST_CLASS, FacadeIsInitiallyEmpty) {
		// Act:
		RunUtFacadeTest([](const auto& facade, const auto&) {
			// Assert:
			AssertEmpty(facade);
		});
	}

	// endregion

	// region apply

	TEST(TEST_CLASS, CanApplyTransactionsToCache_AllSuccess) {
		// Arrange:
		RunUtFacadeTest([](auto& facade, const auto& executionConfig) {
			auto transactionInfos = test::CreateTransactionInfos(4);
			auto transactionHashes = test::ExtractHashes(transactionInfos);

			// Act: apply transactions to the cache
			std::vector<bool> applyResults;
			for (const auto& transactionInfo : transactionInfos)
				applyResults.push_back(facade.apply(transactionInfo));

			// Assert:
			EXPECT_EQ(std::vector<bool>(4, true), applyResults);

			// - all transactions were added to ut cache
			EXPECT_EQ(Default_Height + Height(1), facade.height());
			EXPECT_EQ(4u, facade.size());
			test::AssertEquivalent(transactionInfos, facade.transactionInfos());

			// - check entity infos (hashes): validator and observer should be called for all notifications (2 per transaction)
			std::vector<std::pair<size_t, size_t>> expectedIndexIdPairs{
				{ 0, 1 }, { 0, 2 }, { 1, 1 }, { 1, 2 }, { 2, 1 }, { 2, 2 }, { 3, 1 }, { 3, 2 }
			};
			AssertEntityInfos("validator", executionConfig.pValidator->params(), transactionHashes, expectedIndexIdPairs);
			AssertEntityInfos("observer", executionConfig.pObserver->params(), transactionHashes, expectedIndexIdPairs);

			// - check contexts
			AssertValidatorContexts(executionConfig, { 0, 0, 2, 2, 4, 4, 6, 6 });
			AssertObserverContexts(executionConfig, 8);
		});
	}

	TEST(TEST_CLASS, CanApplyTransactionsToCache_AllFailure) {
		// Arrange:
		RunUtFacadeTest([](auto& facade, const auto& executionConfig) {
			auto transactionInfos = test::CreateTransactionInfos(4);
			auto transactionHashes = test::ExtractHashes(transactionInfos);

			// - mark all failures
			executionConfig.pValidator->setResult(validators::ValidationResult::Failure);

			// Act: apply transactions to the cache
			std::vector<bool> applyResults;
			for (const auto& transactionInfo : transactionInfos)
				applyResults.push_back(facade.apply(transactionInfo));

			// Assert:
			EXPECT_EQ(std::vector<bool>(4, false), applyResults);

			// - no transactions were added to ut cache
			AssertEmpty(facade);

			// - check entity infos (hashes): validator should be called for first notifications; observer should never be called
			AssertEntityInfos("validator", executionConfig.pValidator->params(), transactionHashes, {
				{ 0, 1 }, { 1, 1 }, { 2, 1 }, { 3, 1 }
			});
			AssertEntityInfos("observer", executionConfig.pObserver->params(), transactionHashes, {});

			// - check contexts
			AssertValidatorContexts(executionConfig, { 0, 0, 0, 0 });
			AssertObserverContexts(executionConfig, 0);
		});
	}

	namespace {
		template<typename TAssertEntityInfosAndContexts>
		void AssertCanApplyTransactionsSomeSuccess(size_t idTrigger, TAssertEntityInfosAndContexts assertEntityInfosAndContexts) {
			// Arrange:
			RunUtFacadeTest([idTrigger, assertEntityInfosAndContexts](auto& facade, const auto& executionConfig) {
				auto transactionInfos = test::CreateTransactionInfos(4);
				auto transactionHashes = test::ExtractHashes(transactionInfos);

				// - mark some failures
				executionConfig.pValidator->setResult(validators::ValidationResult::Failure, transactionHashes[1], idTrigger);
				executionConfig.pValidator->setResult(validators::ValidationResult::Failure, transactionHashes[3], idTrigger);

				// Act: apply transactions to the cache
				std::vector<bool> applyResults;
				for (const auto& transactionInfo : transactionInfos)
					applyResults.push_back(facade.apply(transactionInfo));

				// Assert:
				EXPECT_EQ(std::vector<bool>({ true, false, true, false }), applyResults);

				// - only two transactions were added to ut cache
				EXPECT_EQ(Default_Height + Height(1), facade.height());
				EXPECT_EQ(2u, facade.size());

				std::vector<model::TransactionInfo> expectedTransactionInfos;
				expectedTransactionInfos.push_back(transactionInfos[0].copy());
				expectedTransactionInfos.push_back(transactionInfos[2].copy());
				test::AssertEquivalent(expectedTransactionInfos, facade.transactionInfos());

				// - check entity infos and contexts
				assertEntityInfosAndContexts(transactionHashes, executionConfig);
			});
		}
	}

	TEST(TEST_CLASS, CanApplyTransactionsToCache_SomeSuccess_FirstNotificationFails) {
		// Act:
		AssertCanApplyTransactionsSomeSuccess(1, [](const auto& transactionHashes, const auto& executionConfig) {
			// Assert: check entity infos (hashes)
			AssertEntityInfos("validator", executionConfig.pValidator->params(), transactionHashes, {
				{ 0, 1 }, { 0, 2 }, { 1, 1 }, { 2, 1 }, { 2, 2 }, { 3, 1 }
			});
			AssertEntityInfos("observer", executionConfig.pObserver->params(), transactionHashes, {
				{ 0, 1 }, { 0, 2 }, { 2, 1 }, { 2, 2 }
			});

			AssertValidatorContexts(executionConfig, { 0, 0, 2, 2, 2, 4 });
			AssertObserverContexts(executionConfig, 4);
		});
	}

	TEST(TEST_CLASS, CanApplyTransactionsToCache_SomeSuccess_SecondNotificationFails) {
		// Act:
		AssertCanApplyTransactionsSomeSuccess(2, [](const auto& transactionHashes, const auto& executionConfig) {
			// Assert: check entity infos (hashes)
			AssertEntityInfos("validator", executionConfig.pValidator->params(), transactionHashes, {
				{ 0, 1 }, { 0, 2 }, { 1, 1 }, { 1, 2 }, { 2, 1 }, { 2, 2 }, { 3, 1 }, { 3, 2 }
			});
			AssertEntityInfos("observer", executionConfig.pObserver->params(), transactionHashes, {
				{ 0, 1 }, { 0, 2 }, { 2, 1 }, { 2, 2 }
			});

			AssertValidatorContexts(executionConfig, { 0, 0, 2, 2, 2, 2, 4, 4 });
			AssertObserverContexts(executionConfig, 4);
		});
	}

	// endregion

	// region commit - validation

	namespace {
		auto CreateBlockHeaderWithHeight(Height height) {
			auto pBlockHeader = utils::MakeUniqueWithSize<model::Block>(sizeof(model::BlockHeaderV4));
			test::FillWithRandomData({ reinterpret_cast<uint8_t*>(pBlockHeader.get()), sizeof(model::BlockHeaderV4) });
			pBlockHeader->Size = sizeof(model::BlockHeaderV4);
			pBlockHeader->Height = height;
			pBlockHeader->FeeMultiplier = BlockFeeMultiplier();
			pBlockHeader->BlockReceiptsHash = Hash256();
			pBlockHeader->StateHash = Hash256();
			pBlockHeader->FeeInterest = 1;
			pBlockHeader->FeeInterestDenominator = 1;
			pBlockHeader->Version = MakeVersion(model::NetworkIdentifier::Mijin_Test, model::BlockHeader::Current_Version);
			pBlockHeader->setTransactionPayloadSize(0u);
			return pBlockHeader;
		}
	}

	TEST(TEST_CLASS, CommitFailsWhenBlockHeaderIsInconsistentWithCacheState) {
		// Arrange:
		RunUtFacadeTest(4, [](auto& facade, const auto& transactionInfos, const auto&) {
			// - seed facade with four transactions
			for (const auto& transactionInfo : transactionInfos)
				facade.apply(transactionInfo);

			// Act + Assert: commit
			auto pBlockHeader = CreateBlockHeaderWithHeight(Default_Height);
			EXPECT_THROW(facade.commit(*pBlockHeader), catapult_runtime_error);
		});
	}

	TEST(TEST_CLASS, CommitFailsWhenBlockHeaderFailsValidation) {
		// Arrange:
		RunUtFacadeTest(4, [](auto& facade, const auto& transactionInfos, const auto& executionConfig) {
			// - seed facade with four transactions
			for (const auto& transactionInfo : transactionInfos)
				facade.apply(transactionInfo);

			auto transactionHashes = test::ExtractHashes(transactionInfos);
			transactionHashes.push_back(Hash256()); // zero hash is used for block

			// - fail block validation
			executionConfig.pValidator->setResult(validators::ValidationResult::Failure);

			// Act: commit
			auto pBlockHeader = CreateBlockHeaderWithHeight(Default_Height + Height(1));
			auto pBlock = facade.commit(*pBlockHeader);

			// Assert: transactions have been removed
			AssertEmpty(facade);

			// - check entity infos (hashes): validator fails on first block part
			AssertEntityInfos("validator", executionConfig.pValidator->params(), transactionHashes, {
				{ 0, 1 }, { 0, 2 }, { 1, 1 }, { 1, 2 }, { 2, 1 }, { 2, 2 }, { 3, 1 }, { 3, 2 },
				{ 4, 1 }
			});
			AssertEntityInfos("observer", executionConfig.pObserver->params(), transactionHashes, {
				{ 0, 1 }, { 0, 2 }, { 1, 1 }, { 1, 2 }, { 2, 1 }, { 2, 2 }, { 3, 1 }, { 3, 2 }
			});

			// - check contexts
			AssertValidatorContexts(executionConfig, { 0, 0, 2, 2, 4, 4, 6, 6, 8 });
			AssertObserverContexts(executionConfig, 8);

			// - no block was generated
			EXPECT_FALSE(!!pBlock);
		});
	}

	TEST(TEST_CLASS, CommitProducesBlockWhenValidationPasses_WithoutTransactions) {
		// Arrange:
		RunUtFacadeTest(0, [](auto& facade, const auto&, const auto& executionConfig) {
			// Act: commit
			auto pBlockHeader = CreateBlockHeaderWithHeight(Default_Height + Height(1));
			auto pBlock = facade.commit(*pBlockHeader);

			// Assert: transactions have been removed
			AssertEmpty(facade);

			// - check entity infos (hashes): validator and observer should be called for all notifications (2 per transaction and block)
			AssertEntityInfos("validator", executionConfig.pValidator->params(), { Hash256() }, { { 0, 1 }, { 0, 2 } });
			AssertEntityInfos("observer", executionConfig.pObserver->params(), { Hash256() }, { { 0, 1 }, { 0, 2 } });

			// - check contexts
			AssertValidatorContexts(executionConfig, { 0, 0 });
			AssertObserverContexts(executionConfig, 2);

			// - check block
			EXPECT_EQ_MEMORY(pBlockHeader.get(), pBlock.get(), sizeof(model::BlockHeader));
			EXPECT_EQ(0u, model::CalculateBlockTransactionsInfo(*pBlock).Count);
		});
	}

	TEST(TEST_CLASS, CommitProducesBlockWhenValidationPasses_WithTransactions) {
		// Arrange:
		RunUtFacadeTest(4, [](auto& facade, const auto& transactionInfos, const auto& executionConfig) {
			// - seed facade with four transactions
			auto transactionsSize = 0u;
			for (const auto& transactionInfo : transactionInfos) {
				facade.apply(transactionInfo);
				transactionsSize += transactionInfo.pEntity->Size;
			}

			auto transactionHashes = test::ExtractHashes(transactionInfos);
			transactionHashes.push_back(Hash256()); // zero hash is used for block

			// Act: commit (update header size to match expected)
			auto pBlockHeader = CreateBlockHeaderWithHeight(Default_Height + Height(1));
			pBlockHeader->Size += transactionsSize;
			auto pBlock = facade.commit(*pBlockHeader);

			// Assert: transactions have been removed
			AssertEmpty(facade);

			// - check entity infos (hashes): validator and observer should be called for all notifications (2 per transaction and block)
			std::vector<std::pair<size_t, size_t>> expectedIndexIdPairs{
				{ 0, 1 }, { 0, 2 }, { 1, 1 }, { 1, 2 }, { 2, 1 }, { 2, 2 }, { 3, 1 }, { 3, 2 },
				{ 4, 1 }, { 4, 2 }
			};
			AssertEntityInfos("validator", executionConfig.pValidator->params(), transactionHashes, expectedIndexIdPairs);
			AssertEntityInfos("observer", executionConfig.pObserver->params(), transactionHashes, expectedIndexIdPairs);

			// - check contexts
			AssertValidatorContexts(executionConfig, { 0, 0, 2, 2, 4, 4, 6, 6, 8, 8 });
			AssertObserverContexts(executionConfig, 10);

			// - check block
			EXPECT_EQ_MEMORY(pBlockHeader.get(), pBlock.get(), sizeof(model::BlockHeader));
			EXPECT_EQ(4u, model::CalculateBlockTransactionsInfo(*pBlock).Count);

			auto i = 0u;
			for (const auto& transaction : pBlock->Transactions()) {
				EXPECT_EQ(*transactionInfos[i].pEntity, transaction) << "transaction at " << i;
				++i;
			}
		});
	}

	// endregion

	// region FacadeTestContext

	namespace {
		struct FacadeTestContext {
		public:
			FacadeTestContext(const config::BlockchainConfiguration& config, const chain::ExecutionConfiguration& executionConfig)
					: m_pConfigHolder(config::CreateMockConfigurationHolder(config))
					, m_executionConfig(executionConfig)
					, m_cache(test::CreateEmptyCatapultCache(config, CreateCacheConfiguration(m_dbDirGuard.name()))) {
				test::AddMarkerAccount(m_cache);
				setCacheHeight(Default_Height);
			}

		public:
			cache::CatapultCache& cache() {
				return m_cache;
			}

		public:
			void prepareSignerAccount(const Key& signer) {
				auto cacheDelta = m_cache.createDelta();

				auto& accountStateCacheDelta = cacheDelta.sub<cache::AccountStateCache>();
				accountStateCacheDelta.addAccount(signer, Height(1));

				// recalculate the state hash and commit changes
				cacheDelta.calculateStateHash(Default_Height);
				m_cache.commit(Default_Height);
			}

			void prepareTransactionSignerAccounts(const std::vector<model::TransactionInfo>& transactionInfos) {
				for (const auto& transactionInfo : transactionInfos)
					prepareSignerAccount(transactionInfo.pEntity->Signer);
			}

			void setCacheHeight(Height height) {
				auto delta = m_cache.createDelta();
				m_cache.commit(height);
			}

		public:
			model::UniqueEntityPtr<model::Block> generate(
					const model::Block& blockHeader,
					const std::vector<model::TransactionInfo>& transactionInfos) const {
				HarvestingUtFacadeFactory factory(m_cache, m_executionConfig);

				auto pFacade = factory.create(Default_Time);
				if (!pFacade)
					return nullptr;

				for (const auto& transactionInfo : transactionInfos)
					pFacade->apply(transactionInfo);

				return pFacade->commit(blockHeader);
			}

		private:
			static cache::CacheConfiguration CreateCacheConfiguration(const std::string& databaseDirectory) {
				return cache::CacheConfiguration(databaseDirectory, utils::FileSize(), cache::PatriciaTreeStorageMode::Enabled);
			}

		private:
			test::TempDirectoryGuard m_dbDirGuard;
			std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
			chain::ExecutionConfiguration m_executionConfig;
			cache::CatapultCache m_cache;
		};
	}

	// endregion

	// region commit - verifiable settings honored

	namespace {
		std::vector<model::TransactionInfo> CreateTransactionInfosWithIncreasingMultipliers(uint32_t numTransactionInfos) {
			std::vector<std::pair<uint32_t, uint32_t>> sizeMultiplierPairs;
			for (auto i = 0u; i < numTransactionInfos; ++i)
				sizeMultiplierPairs.emplace_back(200, (2 + i) * 10);

			return test::CreateTransactionInfosFromSizeMultiplierPairs(sizeMultiplierPairs);
		}

		Hash256 CalculateEnabledTestReceiptsHash(uint32_t numReceipts) {
			model::BlockStatementBuilder statementBuilder;
			for (auto i = 0u; i < numReceipts; ++i) {
				// MockAggregateNotificationObserver simulates receipt source changes by incrementing primary source id for every
				// new entity (with sequence id 1). This is not identical to real behavior where blocks have id { 0, 0 }.
				if (0 == i % 2)
					statementBuilder.setSource({ i / 2 + 1, 0 });

				model::Receipt receipt{};
				receipt.Size = sizeof(model::Receipt);
				receipt.Type = static_cast<model::ReceiptType>(2 * (i + 1));
				statementBuilder.addTransactionReceipt(receipt);
			}

			return model::CalculateMerkleHash(*statementBuilder.build());
		}

		Hash256 CalculateEnabledTestStateHash(
				const cache::CatapultCache& cache,
				const std::vector<model::TransactionInfo>& transactionInfos,
				const std::vector<size_t>& indexToMultiplierMap = {}) {
			auto cacheDetachableDelta = cache.createDetachableDelta();
			auto cacheDetachedDelta = cacheDetachableDelta.detach();
			auto pCacheDelta = cacheDetachedDelta.tryLock();

			auto i = 1u;
			for (auto& transactionInfo : transactionInfos) {
				auto& accountStateCacheDelta = pCacheDelta->sub<cache::AccountStateCache>();
				auto accountStateIter = accountStateCacheDelta.find(transactionInfo.pEntity->Signer);

				auto multiplier = indexToMultiplierMap.empty() ? i : indexToMultiplierMap[i - 1];
				auto surplus = Amount(multiplier * transactionInfo.pEntity->Size);
				accountStateIter.get().Balances.credit(Currency_Mosaic_Id, Amount(surplus));
				++i;
			}

			return pCacheDelta->calculateStateHash(Default_Height + Height(1)).StateHash;
		}

		template<typename TAssertHashes>
		void RunEnabledTest(StateVerifyOptions verifyOptions, uint32_t numTransactions, TAssertHashes assertHashes) {
			// Arrange: prepare context
			auto config = CreateConfiguration(verifyOptions);

			test::MockExecutionConfiguration executionConfig(config);
			executionConfig.pObserver->enableReceiptGeneration();
			FacadeTestContext context(config, executionConfig.Config);

			auto pBlockHeader = CreateBlockHeaderWithHeight(Default_Height + Height(1));
			pBlockHeader->FeeMultiplier = BlockFeeMultiplier(1);
			context.prepareSignerAccount(pBlockHeader->Signer);

			auto transactionInfos = CreateTransactionInfosWithIncreasingMultipliers(numTransactions);
			context.prepareTransactionSignerAccounts(transactionInfos);

			// Act: apply all transactions
			auto pBlock = context.generate(*pBlockHeader, transactionInfos);

			// Assert: check the block execution dependent hashes
			ASSERT_TRUE(!!pBlock);

			auto expectedReceiptsHash = CalculateEnabledTestReceiptsHash(2 * (numTransactions + 1));
			auto expectedStateHash = CalculateEnabledTestStateHash(context.cache(), transactionInfos);
			assertHashes(expectedReceiptsHash, expectedStateHash, *pBlock);
		}
	}

	TEST(TEST_CLASS, ZeroHashesAreReturnedWhenVerifiableReceiptsAndStateAreDisabled) {
		// Act:
		RunEnabledTest(StateVerifyOptions::None, 3, [](const auto&, const auto&, const auto& block) {
			// Assert:
			EXPECT_EQ(Hash256(), block.BlockReceiptsHash);
			EXPECT_EQ(Hash256(), block.StateHash);
		});
	}

	TEST(TEST_CLASS, NonZeroReceiptsHashIsReturnedWhenVerifiableReceiptsIsEnabled) {
		// Act:
		RunEnabledTest(StateVerifyOptions::Receipts, 3, [](const auto& expectedReceiptsHash, const auto&, const auto& block) {
			// Sanity:
			EXPECT_NE(Hash256(), expectedReceiptsHash);

			// Assert:
			EXPECT_EQ(expectedReceiptsHash, block.BlockReceiptsHash);
			EXPECT_EQ(Hash256(), block.StateHash);
		});
	}

	TEST(TEST_CLASS, NonZeroStateHashIsReturnedWhenVerifiableStateIsEnabled) {
		// Act:
		RunEnabledTest(StateVerifyOptions::State, 3, [](const auto&, const auto& expectedStateHash, const auto& block) {
			// Sanity:
			EXPECT_NE(Hash256(), expectedStateHash);

			// Assert:
			EXPECT_EQ(Hash256(), block.BlockReceiptsHash);
			EXPECT_EQ(expectedStateHash, block.StateHash);
		});
	}

	TEST(TEST_CLASS, NonZeroHashesAreReturnedWhenVerifiableReceiptsAndStateAreEnabled) {
		// Act:
		RunEnabledTest(StateVerifyOptions::All, 3, [](const auto& expectedReceiptsHash, const auto& expectedStateHash, const auto& block) {
			// Sanity:
			EXPECT_NE(Hash256(), expectedReceiptsHash);
			EXPECT_NE(Hash256(), expectedStateHash);

			// Assert:
			EXPECT_EQ(expectedReceiptsHash, block.BlockReceiptsHash);
			EXPECT_EQ(expectedStateHash, block.StateHash);
		});
	}

	// endregion

	// region commit - verifiable state correct after failures

	TEST(TEST_CLASS, NonzeroHashesAreReturnedWhenVerifiableReceiptsAndStateAreEnabledAfterApplyOperationWithSomeFailures) {
		// Arrange: prepare context
		auto config = CreateConfiguration(StateVerifyOptions::All);
		test::MockExecutionConfiguration executionConfig(config);
		executionConfig.pObserver->enableReceiptGeneration();
		executionConfig.pObserver->enableRollbackEmulation();
		FacadeTestContext context(
			config,
			executionConfig.Config
		);

		auto pBlockHeader = CreateBlockHeaderWithHeight(Default_Height + Height(1));
		pBlockHeader->FeeMultiplier = BlockFeeMultiplier(1);
		context.prepareSignerAccount(pBlockHeader->Signer);

		auto transactionInfos = CreateTransactionInfosWithIncreasingMultipliers(5);
		context.prepareTransactionSignerAccounts(transactionInfos);

		// - mark two failures (triggers are intentionally different to maximize coverage)
		executionConfig.pValidator->setResult(validators::ValidationResult::Failure, transactionInfos[1].EntityHash, 2);
		executionConfig.pValidator->setResult(validators::ValidationResult::Failure, transactionInfos[3].EntityHash, 1);

		// Act: apply 5 transactions with two failures
		auto pBlock = context.generate(*pBlockHeader, transactionInfos);

		// Assert: check the block execution dependent hashes
		ASSERT_TRUE(!!pBlock);

		std::vector<model::TransactionInfo> expectedTransactionInfos;
		for (auto i : { 0u, 2u, 4u })
			expectedTransactionInfos.push_back(transactionInfos[i].copy());

		auto expectedReceiptsHash = CalculateEnabledTestReceiptsHash(2 * (3 + 1));
		auto expectedStateHash = CalculateEnabledTestStateHash(context.cache(), expectedTransactionInfos, {
				// multipliers are configured as `index + 1`, but there are gaps because transactions at indexes 1 and 3 were dropped
				// remaining transactions have initial indexes of { 0, 2, 4 }, which implies multipliers of { 1, 3, 5 }
				1, 3, 5
		});

		EXPECT_EQ(expectedReceiptsHash, pBlock->BlockReceiptsHash);
		EXPECT_EQ(expectedStateHash, pBlock->StateHash);
	}

	// endregion
}}
