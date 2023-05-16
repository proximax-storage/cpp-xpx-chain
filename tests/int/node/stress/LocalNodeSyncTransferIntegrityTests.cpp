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

#include <boost/algorithm/string/replace.hpp>
#include "tests/int/node/stress/test/TransactionBuilderNetworkConfigCapability.h"
#include "sdk/src/extensions/BlockExtensions.h"
#include "tests/int/node/stress/test/LocalNodeSyncIntegrityTestUtils.h"
#include "tests/int/node/stress/test/TransactionsBuilder.h"
#include "tests/int/node/stress/test/TransactionBuilderTransferCapability.h"
#include "tests/int/node/stress/test/TransactionBuilderNamespaceCapability.h"
#include "tests/int/node/test/LocalNodeRequestTestUtils.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/nodeps/Nemesis.h"
#include "tests/TestHarness.h"
#include "tests/test/nodeps/data/BasicExtendedNemesisMemoryBlockStorage_data.h"

namespace catapult { namespace local {

#define TEST_CLASS LocalNodeSyncTransferIntegrityTests

	namespace {
		using BlockChainBuilder = test::BlockChainBuilder;
		using Blocks = BlockChainBuilder::Blocks;

		// region traits

		struct SingleBlockTraits {
			static auto GetBlocks(BlockChainBuilder& builder, const test::TransactionsGenerator& transactionsGenerator) {
				return Blocks{ utils::UniqueToShared(builder.asSingleBlock(transactionsGenerator)) };
			}
		};

		struct MultiBlockTraits {
			static auto GetBlocks(BlockChainBuilder& builder, const test::TransactionsGenerator& transactionsGenerator) {
				return builder.asBlockChain(transactionsGenerator);
			}
		};
		namespace {
			using test_types = ::testing::Types<
					std::pair<std::integral_constant<uint32_t,1>, std::integral_constant<uint32_t,1>>,
					std::pair<std::integral_constant<uint32_t,1>, std::integral_constant<uint32_t,2>>
			>;
			// It is not possible for a nemesis account to be version 2 and a newer account to be version 1

			template<typename TBaseAccountVersion>
			struct LocalNodeSyncTransferIntegrityTests : public ::testing::Test {};
		}

		TYPED_TEST_CASE(LocalNodeSyncTransferIntegrityTests, test_types);


#define SINGLE_MULTI_BASED_TEST(TEST_NAME) \
	template<typename TTraits, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	NO_STRESS_TYPED_TEST(TEST_CLASS, TEST_NAME##_SingleBlock) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<SingleBlockTraits, TypeParam::first_type::value, TypeParam::second_type::value>(); } \
	NO_STRESS_TYPED_TEST(TEST_CLASS, TEST_NAME##_BlockChain) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<MultiBlockTraits, TypeParam::first_type::value, TypeParam::second_type::value>(); } \
	template<typename TTraits, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

		// endregion

		// region utils

		void ResignBlock(model::Block& block) {
			for (const auto* pPrivateKeyString : test::Mijin_Test_Private_Keys) {
				auto keyPairSha3 = crypto::KeyPair::FromString(pPrivateKeyString, DerivationScheme::Ed25519_Sha3);
				if (keyPairSha3.publicKey() == block.Signer) {
					extensions::BlockExtensions(test::GetNemesisGenerationHash()).signFullBlock(keyPairSha3, block);
					return;
				}
				//TEMPORARY BAD FIX
				auto keyPairSha2 = crypto::KeyPair::FromString(pPrivateKeyString, DerivationScheme::Ed25519_Sha2);
				if (keyPairSha2.publicKey() == block.Signer) {
					extensions::BlockExtensions(test::GetNemesisGenerationHash()).signFullBlock(keyPairSha2, block);
					return;
				}
			}

			CATAPULT_THROW_RUNTIME_ERROR("unable to find block signer among mijin test private keys");
		}

		// endregion
	}

	// region transfer application (success)

	namespace {

		template<typename TTestContext>
		std::pair<BlockChainBuilder, std::shared_ptr<model::Block>> GenerateNetworkUpgrade(const TTestContext& context,
																						   const test::Accounts& accounts,
																						   BlockChainBuilder& builder,
																						   bool upgrade = false)
		{

			test::TransactionsBuilder transactionsBuilder(accounts);
			auto networkConfigBuilder = transactionsBuilder.template getCapability<test::TransactionBuilderNetworkConfigCapability>();
			mocks::MockMemoryBlockStorage storage([](){return mocks::CreateNemesisBlockElement(test::Extended_Basic_MemoryBlockStorage_NemesisBlockData);});
			auto pNemesisBlockElement = storage.loadBlockElement(Height(1));
			auto configs = extensions::NemesisBlockLoader::ReadNetworkConfigurationAsStrings(pNemesisBlockElement);
			std::string supportedEntities = std::get<1>(configs);
			std::string content  = std::get<0>(configs);
			if(upgrade) boost::algorithm::replace_first(content, "accountVersion = 1", "accountVersion = 2\nminimumAccountVersion = 1");
			networkConfigBuilder->addNetworkConfigUpdate(content, supportedEntities, BlockDuration(1));

			test::ExternalSourceConnection connection;
			auto pUpgradeBlock = utils::UniqueToShared(builder.asSingleBlock(transactionsBuilder));
			test::PushEntity(connection, ionet::PacketType::Push_Block, pUpgradeBlock);
			test::WaitForHeightAndElements(context, Height(2), 1, 1);
			return std::make_pair(builder, pUpgradeBlock);
		}


		template<typename TTraits, typename TTestContext, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions>
		std::vector<Hash256> RunApplyTest(TTestContext& context) {

			// Arrange:
			std::vector<Hash256> stateHashes;
			test::WaitForBoot(context);
			stateHashes.emplace_back(GetStateHash(context));
			auto currentBlockHeight = 1;
			auto expectedBlockElements = 1;
			// Sanity:
			EXPECT_EQ(Height(1), context.height());

			// Prepare requirements

			test::Accounts accounts(crypto::KeyPair::FromString(test::Mijin_Test_Nemesis_Private_Key, 1), 6, TRemainingAccountVersions, TDefaultAccountVersion);

			auto stateHashCalculator = context.createStateHashCalculator();
			auto& cache = context.localNode().cache();
			auto& accountStateCache = cache.template sub<cache::AccountStateCache>();
			BlockChainBuilder builder(accounts, stateHashCalculator, context.configHolder(), &accountStateCache, context.dataDirectory());

			// - maybe upgrade network
			auto networkUpgradePair = GenerateNetworkUpgrade(context,
															 accounts,
															 builder,
															 TRemainingAccountVersions > 1);
			currentBlockHeight++;
			expectedBlockElements++;

			// - prepare transfers (all transfers are dependent on previous transfer)

			test::TransactionsBuilder transactionsBuilder(accounts);
			auto transferBuilder = transactionsBuilder.template getCapability<test::TransactionBuilderTransferCapability>();
			transferBuilder->addTransfer(0, 1, Amount(1'000'000));
			transferBuilder->addTransfer(1, 2, Amount(900'000));
			transferBuilder->addTransfer(2, 3, Amount(700'000));
			transferBuilder->addTransfer(3, 4, Amount(400'000));
			transferBuilder->addTransfer(4, 5, Amount(50'000));

			auto blocks = TTraits::GetBlocks(builder, transactionsBuilder);

			// Act:
			test::ExternalSourceConnection connection;
			auto pIo = test::PushEntities(connection, ionet::PacketType::Push_Block, blocks);
			currentBlockHeight += blocks.size();
			// - wait for the chain height to change and for all height readers to disconnect
			test::WaitForHeightAndElements(context, Height(currentBlockHeight), expectedBlockElements, 1);
			stateHashes.emplace_back(GetStateHash(context));

			// Assert: the cache has expected balances
			test::AssertCurrencyBalances(accounts, context.localNode().cache(), {
				{ 1, Amount(100'000) },
				{ 2, Amount(200'000) },
				{ 3, Amount(300'000) },
				{ 4, Amount(350'000) },
				{ 5, Amount(50'000) }
			});

			return stateHashes;
		}
	}

	SINGLE_MULTI_BASED_TEST(CanApplyTransactions) {
		// Arrange:
		test::StateHashDisabledTestContext context;

		// Act + Assert:
		auto stateHashes = RunApplyTest<TTraits, test::StateHashDisabledTestContext, TDefaultAccountVersion, TRemainingAccountVersions>(context);

		// Assert: all state hashes are zero
		test::AssertAllZero(stateHashes, 2);
	}

	SINGLE_MULTI_BASED_TEST(CanApplyTransactionsWithStateHashEnabled) {
		// Arrange:
		test::StateHashEnabledTestContext context;

		// Act + Assert:
		auto stateHashes = RunApplyTest<TTraits, test::StateHashEnabledTestContext, TDefaultAccountVersion, TRemainingAccountVersions>(context);

		// Assert: all state hashes are nonzero
		test::AssertAllNonZero(stateHashes, 2);
		test::AssertUnique(stateHashes);
	}

	// endregion

	// region transfer application + rollback (success)

	namespace {

		struct BlockChainStateTracker {

			BlockChainStateTracker(BlockChainBuilder activeBuilder, Blocks currentBlocks, uint32_t currentBlockHeight, uint32_t currentBlockElements) :
				activeBuilder(activeBuilder), currentBlocks(currentBlocks), currentBlockHeight(currentBlockHeight), currentBlockElements(currentBlockElements) {

			}


			BlockChainBuilder activeBuilder;
			Blocks currentBlocks;
			uint32_t currentBlockHeight;
			uint32_t currentBlockElements;
		};
		template<typename TTraits, typename TTestContext, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions>
		BlockChainStateTracker PrepareBootAndUpgrade(
				TTestContext& context,
				const test::Accounts& accounts,
				test::StateHashCalculator& stateHashCalculator) {
			// Arrange:
				test::WaitForBoot(context);

				// Sanity:
				EXPECT_EQ(Height(1), context.height());

				auto currentBlockHeight = 1;
				auto expectedBlockElements = 1;
				// Prepare requirements

				auto& cache = context.localNode().cache();
				auto& accountStateCache = cache.template sub<cache::AccountStateCache>();

				BlockChainBuilder builder(accounts, stateHashCalculator, context.configHolder(), &accountStateCache, context.dataDirectory());

				// - maybe upgrade network

				Blocks blocks;
				auto networkUpgradePair = GenerateNetworkUpgrade(context,
																 accounts,
																 builder,
																 TRemainingAccountVersions > 1);
				currentBlockHeight++;
				expectedBlockElements++;
				blocks.push_back(networkUpgradePair.second);


				return BlockChainStateTracker(builder, blocks, currentBlockHeight, expectedBlockElements);

			}
			template<typename TTraits, typename TTestContext, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions>
			BlockChainStateTracker PrepareFiveChainedTransfers(
				TTestContext& context,
				const test::Accounts& accounts,
				test::StateHashCalculator& stateHashCalculator,
				BlockChainStateTracker tracker) {

			// - prepare transfers (all transfers are dependent on previous transfer)
			test::TransactionsBuilder transactionsBuilder(accounts);
			auto transferBuilder = transactionsBuilder.template getCapability<test::TransactionBuilderTransferCapability>();
			transferBuilder->addTransfer(0, 1, Amount(1'000'000));
			transferBuilder->addTransfer(1, 2, Amount(900'000));
			transferBuilder->addTransfer(2, 3, Amount(700'000));
			transferBuilder->addTransfer(3, 4, Amount(400'000));
			transferBuilder->addTransfer(4, 5, Amount(50'000));

			tracker.activeBuilder = tracker.activeBuilder.createChainedBuilder(stateHashCalculator);
			auto blocks = TTraits::GetBlocks(tracker.activeBuilder, transactionsBuilder);
			for(auto block : blocks)
				tracker.currentBlocks.push_back(block);
			// Act:
			test::ExternalSourceConnection connection;
			auto pIo = test::PushEntities(connection, ionet::PacketType::Push_Block, blocks);
			tracker.currentBlockHeight += blocks.size();
			// - wait for the chain height to change and for all height readers to disconnect
			test::WaitForHeightAndElements(context, Height(tracker.currentBlockHeight), tracker.currentBlockElements, 1);
			return tracker;
		}

		template<typename TTraits, typename TTestContext, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions>
		std::vector<Hash256> RunRollbackTest(TTestContext& context) {
			// Arrange:
			std::vector<Hash256> stateHashes;
			// - always use SingleBlockTraits because a push can rollback at most one block
			std::optional<BlockChainStateTracker> tempTracker;
			test::Accounts accounts(crypto::KeyPair::FromString(test::Mijin_Test_Nemesis_Private_Key, 1), 6, TRemainingAccountVersions, TDefaultAccountVersion);
			{
				auto stateHashCalculator = context.createStateHashCalculator();
				tempTracker = PrepareBootAndUpgrade<SingleBlockTraits, TTestContext, TDefaultAccountVersion, TRemainingAccountVersions>(context, accounts, stateHashCalculator);
				PrepareFiveChainedTransfers<SingleBlockTraits, TTestContext, TDefaultAccountVersion, TRemainingAccountVersions>(context, accounts, stateHashCalculator, *tempTracker);
				stateHashes.emplace_back(GetStateHash(context));
			}

			// - prepare transfers (all are from nemesis)
			test::TransactionsBuilder transactionsBuilder(accounts);
			auto transferBuilder = transactionsBuilder.template getCapability<test::TransactionBuilderTransferCapability>();
			transferBuilder->addTransfer(0, 1, Amount(1'000'000));
			transferBuilder->addTransfer(0, 2, Amount(900'000));
			transferBuilder->addTransfer(0, 3, Amount(700'000));
			transferBuilder->addTransfer(0, 4, Amount(400'000));
			transferBuilder->addTransfer(0, 5, Amount(50'000));

			auto stateHashCalculator2 = context.createStateHashCalculator();
			auto& cache = context.localNode().cache();
			auto& accountStateCache = cache.template sub<cache::AccountStateCache>();
			test::SeedStateHashCalculator(stateHashCalculator2, tempTracker->currentBlocks, test::Extended_Basic_MemoryBlockStorage_NemesisBlockData);
			auto builder = tempTracker->activeBuilder.createChainedBuilder(stateHashCalculator2);



			builder.setBlockTimeInterval(utils::TimeSpan::FromSeconds(13)); // better block time will yield better chain
			auto blocks = TTraits::GetBlocks(builder, transactionsBuilder);

			// - prepare a transfer that can only attach to rollback case
			test::TransactionsBuilder transactionsBuilder2(accounts);
			auto transferBuilder2 = transactionsBuilder2.template getCapability<test::TransactionBuilderTransferCapability>();
			transferBuilder2->addTransfer(2, 4, Amount(350'000));

			auto builder2 = builder.createChainedBuilder();
			auto pTailBlock = utils::UniqueToShared(builder2.asSingleBlock(transactionsBuilder2));

			// Act:
			test::ExternalSourceConnection connection;
			auto pIo1 = test::PushEntities(connection, ionet::PacketType::Push_Block, blocks);
			auto pIo2 = test::PushEntity(connection, ionet::PacketType::Push_Block, pTailBlock);

			// - wait for the chain height to change and for all height readers to disconnect
			test::WaitForHeightAndElements(context, Height(2 + blocks.size() + 1), 4, 2);
			stateHashes.emplace_back(GetStateHash(context));

			// Assert: the cache has expected balances
			test::AssertCurrencyBalances(accounts, context.localNode().cache(), {
				{ 1, Amount(1'000'000) },
				{ 2, Amount(550'000) },
				{ 3, Amount(700'000) },
				{ 4, Amount(750'000) },
				{ 5, Amount(50'000) }
			});

			return stateHashes;
		}
	}

	SINGLE_MULTI_BASED_TEST(CanRollbackTransactions) {
		// Arrange:
		test::StateHashDisabledTestContext context;

		// Act + Assert:
		auto stateHashes = RunRollbackTest<TTraits, test::StateHashDisabledTestContext, TDefaultAccountVersion, TRemainingAccountVersions>(context);

		// Assert: all state hashes are zero
		test::AssertAllZero(stateHashes, 2);
	}

	SINGLE_MULTI_BASED_TEST(CanRollbackTransactionsWithStateHashEnabled) {
		// Arrange:
		test::StateHashEnabledTestContext context;

		// Act + Assert:
		auto stateHashes = RunRollbackTest<TTraits, test::StateHashEnabledTestContext, TDefaultAccountVersion, TRemainingAccountVersions>(context);

		// Assert: all state hashes are nonzero
		test::AssertAllNonZero(stateHashes, 2);
		test::AssertUnique(stateHashes);
	}

	// endregion

	// region transfer application (validation failure)

	namespace {
		template<typename TTraits, typename TTestContext, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions, typename TGenerateInvalidBlocks>
		std::vector<Hash256> RunRejectInvalidApplyTest(TTestContext& context, TGenerateInvalidBlocks generateInvalidBlocks) {
			// Arrange:
			std::vector<Hash256> stateHashes;
			test::Accounts accounts(crypto::KeyPair::FromString(test::Mijin_Test_Nemesis_Private_Key, 1), 6,  TRemainingAccountVersions, TDefaultAccountVersion);
			std::unique_ptr<BlockChainBuilder> pBuilder1;
			Blocks seedBlocks;
			{
				// - seed the chain with initial blocks
				auto stateHashCalculator = context.createStateHashCalculator();
				auto tracker = PrepareBootAndUpgrade<SingleBlockTraits, TTestContext, TDefaultAccountVersion, TRemainingAccountVersions>(context, accounts, stateHashCalculator);
				tracker = PrepareFiveChainedTransfers<TTraits, TTestContext, TDefaultAccountVersion, TRemainingAccountVersions>(context, accounts, stateHashCalculator, tracker);
				pBuilder1 = std::make_unique<BlockChainBuilder>(tracker.activeBuilder);
				seedBlocks = tracker.currentBlocks;
				stateHashes.emplace_back(GetStateHash(context));
			}

			Blocks invalidBlocks;
			{
				auto stateHashCalculator = context.createStateHashCalculator();
				test::SeedStateHashCalculator(stateHashCalculator, seedBlocks, test::Extended_Basic_MemoryBlockStorage_NemesisBlockData);

				// - prepare invalid blocks
				auto builder2 = pBuilder1->createChainedBuilder(stateHashCalculator);
				invalidBlocks = generateInvalidBlocks(accounts, builder2);
			}

			std::shared_ptr<model::Block> pTailBlock;
			{
				auto stateHashCalculator = context.createStateHashCalculator();
				test::SeedStateHashCalculator(stateHashCalculator, seedBlocks, test::Extended_Basic_MemoryBlockStorage_NemesisBlockData);

				// - prepare a transfer that can only attach to initial blocks
				test::TransactionsBuilder transactionsBuilder(accounts);
				auto transferBuilder = transactionsBuilder.template getCapability<test::TransactionBuilderTransferCapability>();
				transferBuilder->addTransfer(1, 2, Amount(91'000));

				auto builder3 = pBuilder1->createChainedBuilder(stateHashCalculator);
				pTailBlock = utils::UniqueToShared(builder3.asSingleBlock(transactionsBuilder));
			}

			// Act:
			test::ExternalSourceConnection connection;
			auto pIo1 = test::PushEntities(connection, ionet::PacketType::Push_Block, invalidBlocks);
			auto pIo2 = test::PushEntity(connection, ionet::PacketType::Push_Block, pTailBlock);

			// - wait for the chain height to change and for all height readers to disconnect
			test::WaitForHeightAndElements(context, Height(1 + seedBlocks.size() + 1), 4, 2);
			stateHashes.emplace_back(GetStateHash(context));

			// Assert: the cache has expected balances
			test::AssertCurrencyBalances(accounts, context.localNode().cache(), {
				{ 1, Amount(9'000) },
				{ 2, Amount(291'000) },
				{ 3, Amount(300'000) },
				{ 4, Amount(350'000) },
				{ 5, Amount(50'000) }
			});

			return stateHashes;
		}

		template<typename TTraits, typename TTestContext, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions>
		std::vector<Hash256> RunRejectInvalidValidationApplyTest(TTestContext& context) {
			// Act + Assert:
			return RunRejectInvalidApplyTest<TTraits, TTestContext, TDefaultAccountVersion, TRemainingAccountVersions>(context, [](const auto& accounts, auto& builder) {
				// Arrange: prepare three transfers, where second is invalid
				test::TransactionsBuilder transactionsBuilder(accounts);
				auto transferBuilder = transactionsBuilder.template getCapability<test::TransactionBuilderTransferCapability>();
				transferBuilder->addTransfer(1, 5, Amount(90'000));
				transferBuilder->addTransfer(2, 5, Amount(200'001));
				transferBuilder->addTransfer(3, 5, Amount(80'000));
				return TTraits::GetBlocks(builder, transactionsBuilder);
			});
		}
	}

	SINGLE_MULTI_BASED_TEST(CanRejectInvalidValidationApplyTransactions) {
		// Arrange:
		test::StateHashDisabledTestContext context;

		// Act + Assert:
		auto stateHashes = RunRejectInvalidValidationApplyTest<TTraits, test::StateHashDisabledTestContext, TDefaultAccountVersion, TRemainingAccountVersions>(context);

		// Assert: all state hashes are zero
		test::AssertAllZero(stateHashes, 2);
	}

	SINGLE_MULTI_BASED_TEST(CanRejectInvalidValidationApplyTransactionsWithStateHashEnabled) {
		// Arrange:
		test::StateHashEnabledTestContext context;

		// Act + Assert:
		auto stateHashes = RunRejectInvalidValidationApplyTest<TTraits, test::StateHashEnabledTestContext, TDefaultAccountVersion, TRemainingAccountVersions>(context);

		// Assert: all state hashes are nonzero
		test::AssertAllNonZero(stateHashes, 2);
		test::AssertUnique(stateHashes);
	}

	// endregion

	// region transfer application (state hash failure)

	namespace {
		template<typename TTraits, typename TTestContext, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions>
		std::vector<Hash256> RunRejectInvalidStateHashApplyTest(TTestContext& context) {
			// Act + Assert:
			return RunRejectInvalidApplyTest<TTraits, TTestContext, TDefaultAccountVersion, TRemainingAccountVersions>(context, [](const auto& accounts, auto& builder) {
				// Arrange: prepare three valid transfers
				test::TransactionsBuilder transactionsBuilder(accounts);
				auto transferBuilder = transactionsBuilder.template getCapability<test::TransactionBuilderTransferCapability>();
				transferBuilder->addTransfer(1, 5, Amount(90'000));
				transferBuilder->addTransfer(2, 5, Amount(80'000));
				transferBuilder->addTransfer(3, 5, Amount(80'000));
				auto blocks = TTraits::GetBlocks(builder, transactionsBuilder);

				// - corrupt state hash of last block
				test::FillWithRandomData(blocks.back()->StateHash);
				ResignBlock(*blocks.back());
				return blocks;
			});
		}
	}

	SINGLE_MULTI_BASED_TEST(CanRejectInvalidStateHashApplyTransactions) {
		// Arrange:
		test::StateHashDisabledTestContext context;

		// Act + Assert:
		auto stateHashes = RunRejectInvalidStateHashApplyTest<TTraits, test::StateHashDisabledTestContext, TDefaultAccountVersion, TRemainingAccountVersions>(context);

		// Assert: all state hashes are zero
		test::AssertAllZero(stateHashes, 2);
	}

	SINGLE_MULTI_BASED_TEST(CanRejectInvalidStateHashApplyTransactionsWithStateHashEnabled) {
		// Arrange:
		test::StateHashEnabledTestContext context;

		// Act + Assert:
		auto stateHashes = RunRejectInvalidStateHashApplyTest<TTraits, test::StateHashEnabledTestContext, TDefaultAccountVersion, TRemainingAccountVersions>(context);

		// Assert: all state hashes are nonzero
		test::AssertAllNonZero(stateHashes, 2);
		test::AssertUnique(stateHashes);
	}

	// endregion

	// region transfer application + rollback (validation failure)

	namespace {
		template<typename TTraits, typename TTestContext, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions, typename TGenerateInvalidBlocks>
		std::vector<Hash256> RunRejectInvalidRollbackTest(TTestContext& context, TGenerateInvalidBlocks generateInvalidBlocks) {
			// Arrange:
			std::vector<Hash256> stateHashes;
			test::Accounts accounts(crypto::KeyPair::FromString(test::Mijin_Test_Nemesis_Private_Key, 1), 6,  TRemainingAccountVersions, TDefaultAccountVersion);
			std::unique_ptr<BlockChainBuilder> pBuilder1;
			Blocks seedBlocks;
			{
				// - seed the chain with initial blocks
				// - always use SingleBlockTraits because a push can rollback at most one block
				auto stateHashCalculator = context.createStateHashCalculator();
				auto tracker = PrepareBootAndUpgrade<SingleBlockTraits, TTestContext, TDefaultAccountVersion, TRemainingAccountVersions>(context, accounts, stateHashCalculator);
				tracker = PrepareFiveChainedTransfers<SingleBlockTraits, TTestContext, TDefaultAccountVersion, TRemainingAccountVersions>(context, accounts, stateHashCalculator, tracker);
				pBuilder1 = std::make_unique<BlockChainBuilder>(tracker.activeBuilder);
				seedBlocks = tracker.currentBlocks;
				stateHashes.emplace_back(GetStateHash(context));
			}

			Blocks invalidBlocks;
			{
				auto stateHashCalculator = context.createStateHashCalculator();
				auto& cache = context.localNode().cache();
				auto& accountStateCache = cache.template sub<cache::AccountStateCache>();

				BlockChainBuilder builder2(accounts, stateHashCalculator, context.configHolder(), &accountStateCache, context.dataDirectory());

				// - prepare invalid blocks
				builder2.setBlockTimeInterval(utils::TimeSpan::FromSeconds(13)); // better block time will yield better chain
				invalidBlocks = generateInvalidBlocks(accounts, builder2);
			}

			std::shared_ptr<model::Block> pTailBlock;
			{
				auto stateHashCalculator = context.createStateHashCalculator();
				test::SeedStateHashCalculator(stateHashCalculator, seedBlocks, test::Extended_Basic_MemoryBlockStorage_NemesisBlockData);

				// - prepare a transfer that can only attach to initial blocks
				test::TransactionsBuilder transactionsBuilder(accounts);
				auto transferBuilder = transactionsBuilder.template getCapability<test::TransactionBuilderTransferCapability>();
				transferBuilder->addTransfer(1, 2, Amount(91'000));

				auto builder3 = pBuilder1->createChainedBuilder(stateHashCalculator);
				pTailBlock = utils::UniqueToShared(builder3.asSingleBlock(transactionsBuilder));
			}

			// Act:
			test::ExternalSourceConnection connection;
			auto pIo1 = test::PushEntities(connection, ionet::PacketType::Push_Block, invalidBlocks);
			auto pIo2 = test::PushEntity(connection, ionet::PacketType::Push_Block, pTailBlock);

			// - wait for the chain height to change and for all height readers to disconnect
			test::WaitForHeightAndElements(context, Height(1 + seedBlocks.size() + 1), 4, 2);
			stateHashes.emplace_back(GetStateHash(context));

			// Assert: the cache has expected balances
			test::AssertCurrencyBalances(accounts, context.localNode().cache(), {
				{ 1, Amount(9'000) },
				{ 2, Amount(291'000) },
				{ 3, Amount(300'000) },
				{ 4, Amount(350'000) },
				{ 5, Amount(50'000) }
			});

			return stateHashes;
		}

		template<typename TTraits, typename TTestContext, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions>
		std::vector<Hash256> RunRejectInvalidValidationRollbackTest(TTestContext& context) {
			// Act + Assert:
			return RunRejectInvalidRollbackTest<TTraits, TTestContext, TDefaultAccountVersion, TRemainingAccountVersions>(context, [](const auto& accounts, auto& builder) {
				// Arrange: prepare five transfers, where third is invalid
				test::TransactionsBuilder transactionsBuilder(accounts);
				auto transferBuilder = transactionsBuilder.template getCapability<test::TransactionBuilderTransferCapability>();
				transferBuilder->addTransfer(0, 1, Amount(10'000));
				transferBuilder->addTransfer(0, 2, Amount(900'000));
				transferBuilder->addTransfer(2, 3, Amount(900'001));
				transferBuilder->addTransfer(0, 4, Amount(400'000));
				transferBuilder->addTransfer(0, 5, Amount(50'000));
				return TTraits::GetBlocks(builder, transactionsBuilder);
			});
		}
	}

	SINGLE_MULTI_BASED_TEST(CanRejectInvalidValidationRollbackTransactions) {
		// Arrange:
		test::StateHashDisabledTestContext context;

		// Act + Assert:
		auto stateHashes = RunRejectInvalidValidationRollbackTest<TTraits, test::StateHashDisabledTestContext, TDefaultAccountVersion, TRemainingAccountVersions>(context);

		// Assert: all state hashes are zero
		test::AssertAllZero(stateHashes, 2);
	}

	SINGLE_MULTI_BASED_TEST(CanRejectInvalidValidationRollbackTransactionsWithStateHashEnabled) {
		// Arrange:
		test::StateHashEnabledTestContext context;

		// Act + Assert:
		auto stateHashes = RunRejectInvalidValidationRollbackTest<TTraits, test::StateHashEnabledTestContext, TDefaultAccountVersion, TRemainingAccountVersions>(context);

		// Assert: all state hashes are nonzero
		test::AssertAllNonZero(stateHashes, 2);
		test::AssertUnique(stateHashes);
	}

	// endregion

	// region transfer application + rollback (state hash failure)

	namespace {
		template<typename TTraits, typename TTestContext, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions>
		std::vector<Hash256> RunRejectInvalidStateHashRollbackTest(TTestContext& context) {
			// Act + Assert:
			return RunRejectInvalidRollbackTest<TTraits, TTestContext, TDefaultAccountVersion, TRemainingAccountVersions>(context, [](const auto& accounts, auto& builder) {
				// Arrange: prepare five valid transfers
				test::TransactionsBuilder transactionsBuilder(accounts);
				auto transferBuilder = transactionsBuilder.template getCapability<test::TransactionBuilderTransferCapability>();
				transferBuilder->addTransfer(0, 1, Amount(10'000));
				transferBuilder->addTransfer(0, 2, Amount(900'000));
				transferBuilder->addTransfer(0, 3, Amount(900'001));
				transferBuilder->addTransfer(0, 4, Amount(400'000));
				transferBuilder->addTransfer(0, 5, Amount(50'000));
				auto blocks = TTraits::GetBlocks(builder, transactionsBuilder);

				// - corrupt state hash of last block
				test::FillWithRandomData(blocks.back()->StateHash);
				ResignBlock(*blocks.back());
				return blocks;
			});
		}
	}

	SINGLE_MULTI_BASED_TEST(CanRejectInvalidStateHashRollbackTransactions) {
		// Arrange:
		test::StateHashDisabledTestContext context;

		// Act + Assert:
		auto stateHashes = RunRejectInvalidStateHashRollbackTest<TTraits, test::StateHashDisabledTestContext, TDefaultAccountVersion, TRemainingAccountVersions>(context);

		// Assert: all state hashes are zero
		test::AssertAllZero(stateHashes, 2);
	}

	SINGLE_MULTI_BASED_TEST(CanRejectInvalidStateHashRollbackTransactionsWithStateHashEnabled) {
		// Arrange:
		test::StateHashEnabledTestContext context;

		// Act + Assert:
		auto stateHashes = RunRejectInvalidStateHashRollbackTest<TTraits, test::StateHashEnabledTestContext, TDefaultAccountVersion, TRemainingAccountVersions>(context);

		// Assert: all state hashes are nonzero
		test::AssertAllNonZero(stateHashes, 2);
		test::AssertUnique(stateHashes);
	}

	// endregion
}}
