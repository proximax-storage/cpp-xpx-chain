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
#include "plugins/txes/namespace/src/config/NamespaceConfiguration.h"
#include "tests/int/node/stress/test/ExpiryTestUtils.h"
#include "tests/int/node/stress/test/TransactionsBuilder.h"
#include "tests/int/node/stress/test/TransactionBuilderTransferCapability.h"
#include "tests/int/node/stress/test/LocalNodeSyncIntegrityTestUtils.h"
#include "tests/TestHarness.h"
#include "tests/test/nodeps/data/BasicExtendedNemesisMemoryBlockStorage_data.h"

namespace catapult { namespace local {

#define TEST_CLASS LocalNodeSyncNamespaceIntegrityTests

	namespace {
		using BlockChainBuilder = test::BlockChainBuilder;
		using Blocks = BlockChainBuilder::Blocks;

		// region utils

		using NamespaceStateHashes = std::vector<std::pair<Hash256, Hash256>>;

		Hash256 GetComponentStateHash(const test::PeerLocalNodeTestContext& context) {
			auto subCacheMerkleRoots = context.localNode().cache().createView().calculateStateHash().SubCacheMerkleRoots;
			return subCacheMerkleRoots.empty() ? Hash256() : subCacheMerkleRoots[2]; // { Config, AccountState, *Namespace*, Mosaic }
		}
		template<typename TTestContext>
		std::pair<BlockChainBuilder, std::shared_ptr<model::Block>> GenerateNetworkUpgrade(TTestContext& context,
																						   const test::Accounts& accounts,
																						   test::StateHashCalculator& stateHashCalculator,
																						   test::ExternalSourceConnection& connection,
																						   bool isV2 = false)
		{

			test::TransactionsBuilder transactionsBuilder(accounts);
			auto networkConfigBuilder = transactionsBuilder.template getCapability<test::TransactionBuilderNetworkConfigCapability>();
			mocks::MockMemoryBlockStorage storage([](){return mocks::CreateNemesisBlockElement(test::Extended_Basic_MemoryBlockStorage_NemesisBlockData);});
			auto pNemesisBlockElement = storage.loadBlockElement(Height(1));
			auto configs = extensions::NemesisBlockLoader::ReadNetworkConfigurationAsStrings(pNemesisBlockElement);
			std::string supportedEntities = std::get<1>(configs);
			std::string content  = std::get<0>(configs);
			if(isV2)
				boost::algorithm::replace_first(content, "accountVersion = 1", "accountVersion = 2\nminimumAccountVersion = 1");
			boost::algorithm::replace_first(content, "namespaceGracePeriodDuration = 0d", "namespaceGracePeriodDuration = 1h");
			networkConfigBuilder->addNetworkConfigUpdate(content, supportedEntities, BlockDuration(1));
			auto& cache = context.localNode().cache();
			auto& accountStateCache = cache.template sub<cache::AccountStateCache>();
			BlockChainBuilder builder(accounts, stateHashCalculator, context.configHolder(), &accountStateCache, context.dataDirectory());
			auto pUpgradeBlock = utils::UniqueToShared(builder.asSingleBlock(transactionsBuilder));
			test::PushEntity(connection, ionet::PacketType::Push_Block, pUpgradeBlock);
			test::WaitForHeightAndElements(context, Height(2), 1, 1);
			return std::make_pair(builder, pUpgradeBlock);
		}

		template<typename TTestContext>
		void AssertBooted(const TTestContext& context, Height expectedHeight) {
			// Assert:
			EXPECT_EQ(Height(expectedHeight), context.height());
			test::AssertNamespaceCount(context.localNode(), 1);
		}

		template<typename TTestContext>
		std::pair<BlockChainBuilder, std::shared_ptr<model::Block>> PrepareTwoRootNamespaces(
				BlockChainBuilder& builder,
				Height expectedHeightOnDone,
				TTestContext& context,
				const test::Accounts& accounts,
				test::ExternalSourceConnection& connection,
				test::StateHashCalculator& stateHashCalculator,
				NamespaceStateHashes& stateHashes) {
			// Arrange:
			test::WaitForBoot(context);
			stateHashes.emplace_back(GetStateHash(context), GetComponentStateHash(context));

			// Sanity:
			AssertBooted(context, expectedHeightOnDone-Height(1));

			// - prepare namespace registrations
			test::TransactionsBuilder transactionsBuilder(accounts);
			auto transferBuilder = transactionsBuilder.template getCapability<test::TransactionBuilderTransferCapability>();
			auto namespaceBuilder = transactionsBuilder.template getCapability<test::TransactionBuilderNamespaceCapability>();
			namespaceBuilder->addNamespace(0, "foo", BlockDuration(12));
			namespaceBuilder->addNamespace(0, "bar", BlockDuration(12));

			auto pNamespaceBlock = utils::UniqueToShared(builder.asSingleBlock(transactionsBuilder));

			// Act:
			auto pIo = test::PushEntity(connection, ionet::PacketType::Push_Block, pNamespaceBlock);

			// - wait for the chain height to change and for all height readers to disconnect
			test::WaitForHeightAndElements(context, Height(expectedHeightOnDone), expectedHeightOnDone.unwrap()-1, 1);
			stateHashes.emplace_back(GetStateHash(context), GetComponentStateHash(context));

			// Sanity: the cache has expected namespaces
			test::AssertNamespaceCount(context.localNode(), 3);

			return std::make_pair(builder, std::move(pNamespaceBlock));
		}

		struct TransferBlocksResult {
			BlockChainBuilder Builder;
			Blocks AllBlocks;
			uint32_t NumAliveChains;
		};

		template<typename TTestContext>
		TransferBlocksResult PushTransferBlocks(
				TTestContext& context,
				Height initialHeight,
				test::ExternalSourceConnection& connection,
				BlockChainBuilder& builder,
				size_t numTotalBlocks,
				const test::Accounts& accounts) {
			Blocks allBlocks;
			auto numAliveChains = 0u;
			auto numRemainingBlocks = numTotalBlocks;
			for (;;) {
				test::TransactionsBuilder transactionsBuilder(accounts);
				auto transferBuilder = transactionsBuilder.template getCapability<test::TransactionBuilderTransferCapability>();
				auto numBlocks = std::min<size_t>(50, numRemainingBlocks);
				for (auto i = 0u; i < numBlocks; ++i)
					transferBuilder->addTransfer(0, 1, Amount(1));

				auto blocks = builder.asBlockChain(transactionsBuilder);
				auto pIo = test::PushEntities(connection, ionet::PacketType::Push_Block, blocks);

				numRemainingBlocks -= numBlocks;
				++numAliveChains;
				allBlocks.insert(allBlocks.end(), blocks.cbegin(), blocks.cend());

				test::WaitForHeightAndElements(context, Height(initialHeight.unwrap() + numTotalBlocks - numRemainingBlocks), initialHeight.unwrap() - 1 + numAliveChains, 1);
				if (0 == numRemainingBlocks)
					break;

				builder = builder.createChainedBuilder();
			}

			return TransferBlocksResult{ builder, allBlocks, numAliveChains };
		}

		// endregion
	}

	namespace {
		using test_types = ::testing::Types<
				std::pair<std::integral_constant<uint32_t,1>, std::integral_constant<uint32_t,2>>, //RUN THIS INSTEAD IF THE MIJIN TEST ACCOUNTS ARE V1 ACCOUNTS
				//std::pair<std::integral_constant<uint32_t,2>, std::integral_constant<uint32_t,2>>, //RUN THIS INSTEAD IF THE MIJIN TEST ACCOUNTS ARE V2 ACCOUNTS
				std::pair<std::integral_constant<uint32_t,1>, std::integral_constant<uint32_t,1>>
				>;
		// It is not possible for a nemesis account to be version 2 and a newer account to be version 1

		template<typename TBaseAccountVersion>
		struct LocalNodeSyncNamespaceIntegrityTests : public ::testing::Test {};
	}

	TYPED_TEST_CASE(LocalNodeSyncNamespaceIntegrityTests, test_types);
	// region namespace (register)

	namespace {
		template<typename TTestContext, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions>
		NamespaceStateHashes RunRegisterNamespaceTest(TTestContext& context) {
			// Arrange:
			NamespaceStateHashes stateHashes;
			test::WaitForBoot(context);
			stateHashes.emplace_back(GetStateHash(context), GetComponentStateHash(context));

			// Sanity:
			AssertBooted(context, Height(1));

			// - prepare namespace registrations
			test::Accounts accounts(1, TRemainingAccountVersions, TDefaultAccountVersion);
			auto stateHashCalculator = context.createStateHashCalculator();
			auto& cache = context.localNode().cache();
			auto& accountStateCache = cache.template sub<cache::AccountStateCache>();

			BlockChainBuilder builder(accounts, stateHashCalculator, context.configHolder(), &accountStateCache, context.dataDirectory());
			test::TransactionsBuilder transactionsBuilder(accounts);
			auto namespaceBuilder = transactionsBuilder.template getCapability<test::TransactionBuilderNamespaceCapability>();
			namespaceBuilder->addNamespace(0, "foo", BlockDuration(10));
			namespaceBuilder->addNamespace(0, "bar", BlockDuration(10));
			auto pNamespaceBlock = utils::UniqueToShared(builder.asSingleBlock(transactionsBuilder));

			// Act:
			test::ExternalSourceConnection connection;
			auto pIo = test::PushEntity(connection, ionet::PacketType::Push_Block, pNamespaceBlock);

			// - wait for the chain height to change and for all height readers to disconnect
			test::WaitForHeightAndElements(context, Height(2), 1, 1);
			stateHashes.emplace_back(GetStateHash(context), GetComponentStateHash(context));

			// Assert: the cache has expected namespaces
			test::AssertNamespaceCount(context.localNode(), 3);

			return stateHashes;
		}
	}

	NO_STRESS_TYPED_TEST(LocalNodeSyncNamespaceIntegrityTests, CanRegisterNamespace) {
		// Arrange:
		test::StateHashDisabledTestContext context;

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunRegisterNamespaceTest<test::StateHashDisabledTestContext, TypeParam::first_type::value, TypeParam::second_type::value>(context));

		// Assert:
		test::AssertAllZero(stateHashesPair, 2);
	}

	NO_STRESS_TYPED_TEST(LocalNodeSyncNamespaceIntegrityTests, CanRegisterNamespaceWithStateHashEnabled) {
		// Arrange:
		test::StateHashEnabledTestContext context;

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunRegisterNamespaceTest<test::StateHashEnabledTestContext, TypeParam::first_type::value, TypeParam::second_type::value>(context));

		// Assert: all state hashes are nonzero
		test::AssertAllNonZero(stateHashesPair.first, 2);
		test::AssertUnique(stateHashesPair.first);

		// - all namespace cache merkle roots are nonzero
		test::AssertAllNonZero(stateHashesPair.second, 2);
		test::AssertUnique(stateHashesPair.second);
	}

	// endregion

	// region namespace (expire)

	namespace {
		// namespace expires after the sum of the following
		//   (1) namespace duration ==> 12
		constexpr auto Blocks_Before_Namespace_Expire = static_cast<uint32_t>(12);

		template<typename TTestContext, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions>
		NamespaceStateHashes RunNamespaceStateChangeTest(TTestContext& context, size_t numAliveBlocks, size_t numExpectedNamespaces) {
			// Arrange:
			NamespaceStateHashes stateHashes;
			test::Accounts accounts(crypto::KeyPair::FromString(test::Mijin_Test_Nemesis_Private_Key, 1), 2, TRemainingAccountVersions, TDefaultAccountVersion);
			auto stateHashCalculator = context.createStateHashCalculator();


			test::ExternalSourceConnection connection;
			auto builderNetworkPair = GenerateNetworkUpgrade(context, accounts, stateHashCalculator, connection, (accounts.cbegin()+1)->second == 2);
			auto builder1 = builderNetworkPair.first.createChainedBuilder();
			auto builderBlockPair = PrepareTwoRootNamespaces(builder1, Height(3), context, accounts, connection, stateHashCalculator, stateHashes);

			// - add the specified number of blocks up to a state change
			auto builder2 = builderBlockPair.first.createChainedBuilder();
			auto transferBlocksResult = PushTransferBlocks(context, Height(3), connection, builder2, numAliveBlocks, accounts);
			auto numAliveChains = transferBlocksResult.NumAliveChains;
			stateHashes.emplace_back(GetStateHash(context), GetComponentStateHash(context));

			// Sanity: all namespaces are still present
			test::AssertNamespaceCount(context.localNode(), 3);

			// - prepare a block that triggers a state change
			auto builder3 = transferBlocksResult.Builder.createChainedBuilder();
			test::TransactionsBuilder transactionsBuilder(accounts);
			auto transferBuilder = transactionsBuilder.template getCapability<test::TransactionBuilderTransferCapability>();
			transferBuilder->addTransfer(0, 1, Amount(1));
			auto pTailBlock = utils::UniqueToShared(builder3.asSingleBlock(transactionsBuilder));

			// Act:
			auto pIo1 = test::PushEntity(connection, ionet::PacketType::Push_Block, pTailBlock);

			// - wait for the chain height to change and for all height readers to disconnect
			test::WaitForHeightAndElements(context, Height(3 + numAliveBlocks + 1), 2 + numAliveChains + 1, 1);
			stateHashes.emplace_back(GetStateHash(context), GetComponentStateHash(context));

			// Assert: the cache has the expected balances and namespaces
			test::AssertCurrencyBalances(accounts, context.localNode().cache(), {
				{ 1, Amount(numAliveBlocks + 1) }
			});
			test::AssertNamespaceCount(context.localNode(), numExpectedNamespaces);

			return stateHashes;
		}

		template<typename TTestContext, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions>
		NamespaceStateHashes RunExpireNamespaceTest(TTestContext& context) {
			// Act:
			return RunNamespaceStateChangeTest<TTestContext,TDefaultAccountVersion,TRemainingAccountVersions>(context, Blocks_Before_Namespace_Expire - 1, 3);
		}
	}

	NO_STRESS_TYPED_TEST(LocalNodeSyncNamespaceIntegrityTests, CanExpireNamespace) {
		// Arrange:
		test::StateHashDisabledTestContext context;

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunExpireNamespaceTest<test::StateHashDisabledTestContext, TypeParam::first_type::value, TypeParam::second_type::value>(context));

		// Assert:
		test::AssertAllZero(stateHashesPair, 4);
	}

	NO_STRESS_TYPED_TEST(LocalNodeSyncNamespaceIntegrityTests, CanExpireNamespaceWithStateHashEnabled) {
		// Arrange:
		test::StateHashEnabledTestContext context;

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunExpireNamespaceTest<test::StateHashEnabledTestContext, TypeParam::first_type::value, TypeParam::second_type::value>(context));

		// Assert: all state hashes are nonzero (since importance is recalculated every block none of the hashes are the same)
		test::AssertAllNonZero(stateHashesPair.first, 4);
		test::AssertUnique(stateHashesPair.first);

		// - all namespace cache merkle roots are nonzero
		test::AssertAllNonZero(stateHashesPair.second, 4);
		EXPECT_EQ(stateHashesPair.second[1], stateHashesPair.second[2]); // hash includes new namespaces
		EXPECT_EQ(stateHashesPair.second[2], stateHashesPair.second[3]); // hash includes new namespaces (expired but in grace period)
		EXPECT_NE(stateHashesPair.second[0], stateHashesPair.second[3]);
	}

	// endregion

	// region namespace (prune)

	namespace {

		uint32_t GetBlocksBeforeNamespacePrune(uint64_t graceBlocks, uint32_t maxRollbackBlocks)
		{
			// namespace is pruned after the sum of the following
			//   (1) namespace duration ==> 12
			//   (2) grace period ========> 1hr of blocks with blockTime target time
			//   (3) max rollback blocks => maxRollbackBlocks
			return static_cast<uint32_t>(12 +  graceBlocks + maxRollbackBlocks);
		}
		template<typename TTestContext, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions>
		NamespaceStateHashes RunPruneNamespaceTest(TTestContext& context) {
			// Act:
			model::NetworkConfiguration config = context.configHolder()->Config().Network;
			return RunNamespaceStateChangeTest<TTestContext,TDefaultAccountVersion,TRemainingAccountVersions>(context, GetBlocksBeforeNamespacePrune(utils::TimeSpan::FromHours(1).seconds() / config.BlockGenerationTargetTime.seconds(), config.MaxRollbackBlocks) - 1, 1);
		}
	}

	NO_STRESS_TYPED_TEST(LocalNodeSyncNamespaceIntegrityTests, CanPruneNamespace) {
		// Arrange:
		test::StateHashDisabledTestContext context;

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunPruneNamespaceTest<test::StateHashDisabledTestContext, TypeParam::first_type::value, TypeParam::second_type::value>(context));

		// Assert:
		test::AssertAllZero(stateHashesPair, 4);
	}

	NO_STRESS_TYPED_TEST(LocalNodeSyncNamespaceIntegrityTests, CanPruneNamespaceWithStateHashEnabled) {
		// Arrange:
		test::StateHashEnabledTestContext context;

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunPruneNamespaceTest<test::StateHashEnabledTestContext, TypeParam::first_type::value, TypeParam::second_type::value>(context));

		// Assert: all state hashes are nonzero (since importance is recalculated every block none of the hashes are the same)
		test::AssertAllNonZero(stateHashesPair.first, 4);
		test::AssertUnique(stateHashesPair.first);

		// - all namespace cache merkle roots are nonzero
		test::AssertAllNonZero(stateHashesPair.second, 4);
		EXPECT_EQ(stateHashesPair.second[0], stateHashesPair.second[2]); // hash does not include new namespaces (expired)
		EXPECT_EQ(stateHashesPair.second[0], stateHashesPair.second[3]); // hash does not include new namespaces (pruned)
	}

	// endregion

	// region namespace (register + deactivate) :: single chain part

	namespace {
		// namespace is deactivated after the sum of the following
		//   (1) namespace duration ==> 12
		//   (2) grace period ========> 1hr of blocks with 60s target time
		constexpr auto Blocks_Before_Namespace_Deactivate = static_cast<uint32_t>(12 + (utils::TimeSpan::FromHours(1).seconds() / 60));

		template<typename TTestContext, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions>
		NamespaceStateHashes RunRegisterAndDeactivateNamespaceTest(TTestContext& context, size_t numAliveBlocks) {
			// Arrange:
			NamespaceStateHashes stateHashes;
			test::Accounts accounts(2, TRemainingAccountVersions, TDefaultAccountVersion);
			auto stateHashCalculator = context.createStateHashCalculator();

			// *** customization of PrepareTwoRootNamespaces ***
			// - create namespace registration block
			test::WaitForBoot(context);
			stateHashes.emplace_back(GetStateHash(context), GetComponentStateHash(context));

			// Sanity:
			AssertBooted(context, Height(1));

			// - prepare namespace registrations
			auto& cache = context.localNode().cache();
			auto& accountStateCache = cache.template sub<cache::AccountStateCache>();

			BlockChainBuilder builder(accounts, stateHashCalculator, context.configHolder(), &accountStateCache, context.dataDirectory());
			test::TransactionsBuilder transactionsBuilder(accounts);
			auto namespaceBuilder = transactionsBuilder.template getCapability<test::TransactionBuilderNamespaceCapability>();
			namespaceBuilder->addNamespace(0, "foo", BlockDuration(12));
			namespaceBuilder->addNamespace(0, "bar", BlockDuration(12));
			auto pNamespaceBlock = utils::UniqueToShared(builder.asSingleBlock(transactionsBuilder));

			// - add the specified number of blocks up to and including a state change
			test::ExternalSourceConnection connection;
			auto builder2 = builder.createChainedBuilder();
			test::TransactionsBuilder transactionsBuilder2(accounts);
			auto transferBuilder = transactionsBuilder2.template getCapability<test::TransactionBuilderTransferCapability>();

			for (auto i = 0u; i <= numAliveBlocks; ++i)
				transferBuilder->addTransfer(0, 1, Amount(1));

			auto blocks = builder2.asBlockChain(transactionsBuilder2);
			blocks.insert(blocks.begin(), pNamespaceBlock);
			test::PushEntities(connection, ionet::PacketType::Push_Block, blocks);
			test::WaitForHeightAndElements(context, Height(2 + numAliveBlocks + 1), 1, 1);
			stateHashes.emplace_back(GetStateHash(context), GetComponentStateHash(context));

			// Assert: the cache has the expected balances and namespaces
			test::AssertCurrencyBalances(accounts, context.localNode().cache(), {
				{ 1, Amount(numAliveBlocks + 1) }
			});
			test::AssertNamespaceCount(context.localNode(), 3);

			return stateHashes;
		}

		template<typename TTestContext, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions>
		NamespaceStateHashes RunRegisterAndDeactivateNamespaceTest(TTestContext& context) {
			// Act:
			return RunRegisterAndDeactivateNamespaceTest<TTestContext,TDefaultAccountVersion,TRemainingAccountVersions>(context, Blocks_Before_Namespace_Deactivate - 1);
		}
	}

	NO_STRESS_TYPED_TEST(LocalNodeSyncNamespaceIntegrityTests, CanRegisterAndDeactivateNamespace_SingleChainPart) {
		// Arrange:
		test::StateHashDisabledTestContext context;

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunRegisterAndDeactivateNamespaceTest<test::StateHashDisabledTestContext, TypeParam::first_type::value, TypeParam::second_type::value>(context));

		// Assert: all state hashes are zero
		test::AssertAllZero(stateHashesPair.first, 2);

		// - all namespace cache merkle roots are zero
		test::AssertAllZero(stateHashesPair.second, 2);
	}

	NO_STRESS_TYPED_TEST(LocalNodeSyncNamespaceIntegrityTests, CanRegisterAndDeactivateNamespaceWithStateHashEnabled_SingleChainPart) {
		// Arrange:
		test::StateHashEnabledTestContext context;

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunRegisterAndDeactivateNamespaceTest<test::StateHashEnabledTestContext, TypeParam::first_type::value, TypeParam::second_type::value>(context));

		// Assert: all state hashes are nonzero (since importance is recalculated every block none of the hashes are the same)
		test::AssertAllNonZero(stateHashesPair.first, 2);
		test::AssertUnique(stateHashesPair.first);

		// - all namespace cache merkle roots are nonzero
		test::AssertAllNonZero(stateHashesPair.second, 2);
		EXPECT_EQ(stateHashesPair.second[0], stateHashesPair.second[1]); // hash doesn't include new namespaces
	}

	// endregion

	// region namespace (expire + rollback)

	namespace {
		template<typename TTestContext, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions>
		NamespaceStateHashes RunNamespaceStateChangeRollbackTest(
				TTestContext& context,
				size_t numAliveBlocks,
				size_t numExpectedNamespaces) {
			// Arrange:
			NamespaceStateHashes stateHashes;
			test::Accounts accounts(crypto::KeyPair::FromString(test::Mijin_Test_Nemesis_Private_Key, 1), 2, TRemainingAccountVersions, TDefaultAccountVersion);
			std::unique_ptr<BlockChainBuilder> pBuilder;
			std::vector<std::shared_ptr<model::Block>> allBlocks;
			uint32_t numAliveChains;
			{
				auto stateHashCalculator = context.createStateHashCalculator();
				test::ExternalSourceConnection connection;
				auto builderNetworkPair = GenerateNetworkUpgrade(context, accounts, stateHashCalculator, connection, (accounts.cbegin()+1)->second == 2);
				auto builder1 = builderNetworkPair.first.createChainedBuilder();
				auto builderBlockPair = PrepareTwoRootNamespaces(builder1, Height(3), context, accounts, connection, stateHashCalculator, stateHashes);

				// - add the specified number of blocks up to a state change
				auto builder2 = builderBlockPair.first.createChainedBuilder();
				auto transferBlocksResult = PushTransferBlocks(context, Height(3), connection, builder2, numAliveBlocks, accounts);
				numAliveChains = transferBlocksResult.NumAliveChains;
				stateHashes.emplace_back(GetStateHash(context), GetComponentStateHash(context));

				// Sanity: all namespaces are still present
				test::AssertNamespaceCount(context.localNode(), 3);

				allBlocks.emplace_back(builderNetworkPair.second);
				allBlocks.emplace_back(builderBlockPair.second);
				allBlocks.insert(allBlocks.end(), transferBlocksResult.AllBlocks.cbegin(), transferBlocksResult.AllBlocks.cend());
				pBuilder = std::make_unique<BlockChainBuilder>(builder2);
			}

			// - prepare a block that triggers a state change
			std::shared_ptr<model::Block> pExpiryBlock1;
			{
				auto stateHashCalculator = context.createStateHashCalculator();
				test::SeedStateHashCalculator(stateHashCalculator, allBlocks, test::Extended_Basic_MemoryBlockStorage_NemesisBlockData);

				auto builder3 = pBuilder->createChainedBuilder(stateHashCalculator);
				test::TransactionsBuilder transactionsBuilder(accounts);
				auto transferBuilder = transactionsBuilder.template getCapability<test::TransactionBuilderTransferCapability>();
				transferBuilder->addTransfer(0, 1, Amount(1));
				pExpiryBlock1 = utils::UniqueToShared(builder3.asSingleBlock(transactionsBuilder));
			}

			// - prepare two blocks that triggers a state change
			Blocks expiryBlocks2;
			{
				auto stateHashCalculator = context.createStateHashCalculator();
				test::SeedStateHashCalculator(stateHashCalculator, allBlocks, test::Extended_Basic_MemoryBlockStorage_NemesisBlockData);

				auto builder3 = pBuilder->createChainedBuilder(stateHashCalculator);
				builder3.setBlockTimeInterval(utils::TimeSpan::FromSeconds(58)); // better block time will yield better chain
				test::TransactionsBuilder transactionsBuilder(accounts);
				auto transferBuilder = transactionsBuilder.template getCapability<test::TransactionBuilderTransferCapability>();
				transferBuilder->addTransfer(0, 1, Amount(1));
				transferBuilder->addTransfer(0, 1, Amount(1));
				expiryBlocks2 = builder3.asBlockChain(transactionsBuilder);
			}

			// Act:
			test::ExternalSourceConnection connection;
			auto pIo1 = test::PushEntity(connection, ionet::PacketType::Push_Block, pExpiryBlock1);
			auto pIo2 = test::PushEntities(connection, ionet::PacketType::Push_Block, expiryBlocks2);

			// - wait for the chain height to change and for all height readers to disconnect
			test::WaitForHeightAndElements(context, Height(3 + numAliveBlocks + 2), 1 + 1 +  numAliveChains + 2, 2);
			stateHashes.emplace_back(GetStateHash(context), GetComponentStateHash(context));

			// Assert: the cache has the expected balances and namespaces
			test::AssertCurrencyBalances(accounts, context.localNode().cache(), {
				{ 1, Amount(numAliveBlocks + 2) }
			});
			test::AssertNamespaceCount(context.localNode(), numExpectedNamespaces);

			return stateHashes;
		}

		template<typename TTestContext, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions>
		NamespaceStateHashes RunExpireAndRollbackNamespaceTest(TTestContext& context) {
			// Act:
			return RunNamespaceStateChangeRollbackTest<TTestContext,TDefaultAccountVersion,TRemainingAccountVersions>(context, Blocks_Before_Namespace_Expire - 1, 3);
		}
	}

	NO_STRESS_TYPED_TEST(LocalNodeSyncNamespaceIntegrityTests, CanExpireAndRollbackNamespace) {
		// Arrange:
		test::StateHashDisabledTestContext context;

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunExpireAndRollbackNamespaceTest<test::StateHashDisabledTestContext, TypeParam::first_type::value, TypeParam::second_type::value>(context));

		// Assert:
		test::AssertAllZero(stateHashesPair, 4);
	}

	NO_STRESS_TYPED_TEST(LocalNodeSyncNamespaceIntegrityTests, CanExpireAndRollbackNamespaceWithStateHashEnabled) {
		// Arrange:
		test::StateHashEnabledTestContext context;

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunExpireAndRollbackNamespaceTest<test::StateHashEnabledTestContext, TypeParam::first_type::value, TypeParam::second_type::value>(context));

		// Assert: all state hashes are nonzero (since importance is recalculated every block none of the hashes are the same)
		test::AssertAllNonZero(stateHashesPair.first, 4);
		test::AssertUnique(stateHashesPair.first);

		// - all namespace cache merkle roots are nonzero
		test::AssertAllNonZero(stateHashesPair.second, 4);
		EXPECT_EQ(stateHashesPair.second[1], stateHashesPair.second[2]); // hash includes new namespaces
		EXPECT_EQ(stateHashesPair.second[2], stateHashesPair.second[3]); // hash includes new namespaces (expired but in grace period)
		EXPECT_NE(stateHashesPair.second[0], stateHashesPair.second[3]);
	}

	// endregion

	// region namespace (prune + rollback)

	namespace {
		template<typename TTestContext, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions>
		NamespaceStateHashes RunPruneAndRollbackNamespaceTest(TTestContext& context) {
			// Act:
			model::NetworkConfiguration config = context.configHolder()->Config().Network;
			return RunNamespaceStateChangeRollbackTest<TTestContext,TDefaultAccountVersion,TRemainingAccountVersions>(context, GetBlocksBeforeNamespacePrune(utils::TimeSpan::FromHours(1).seconds() / config.BlockGenerationTargetTime.seconds(), config.MaxRollbackBlocks) - 1, 1);
		}
	}

	NO_STRESS_TYPED_TEST(LocalNodeSyncNamespaceIntegrityTests, CanPruneAndRollbackNamespace) {
		// Arrange:
		test::StateHashDisabledTestContext context;

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunPruneAndRollbackNamespaceTest<test::StateHashDisabledTestContext, TypeParam::first_type::value, TypeParam::second_type::value>(context));

		// Assert:
		test::AssertAllZero(stateHashesPair, 4);
	}

	NO_STRESS_TYPED_TEST(LocalNodeSyncNamespaceIntegrityTests, CanPruneAndRollbackNamespaceWithStateHashEnabled) {
		// Arrange:
		test::StateHashEnabledTestContext context;

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunPruneAndRollbackNamespaceTest<test::StateHashEnabledTestContext, TypeParam::first_type::value, TypeParam::second_type::value>(context));

		// Assert: all state hashes are nonzero (since importance is recalculated every block none of the hashes are the same)
		test::AssertAllNonZero(stateHashesPair.first, 4);
		test::AssertUnique(stateHashesPair.first);

		// - all namespace cache merkle roots are nonzero
		test::AssertAllNonZero(stateHashesPair.second, 4);
		EXPECT_EQ(stateHashesPair.second[0], stateHashesPair.second[2]); // hash does not include new namespaces (expired)
		EXPECT_EQ(stateHashesPair.second[0], stateHashesPair.second[3]); // hash does not include new namespaces (pruned)
	}

	// endregion
}}
