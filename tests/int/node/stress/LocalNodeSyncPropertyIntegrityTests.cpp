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

#include "tests/int/node/stress/test/ExpiryTestUtils.h"
#include "tests/int/node/stress/test/LocalNodeSyncIntegrityTestUtils.h"
#include "tests/int/node/stress/test/PropertyTransactionsBuilder.h"
#include "tests/TestHarness.h"

namespace catapult { namespace local {

#define TEST_CLASS LocalNodeSyncPropertyIntegrityTests

	namespace {
		using BlockChainBuilder = test::BlockChainBuilder;
		using Blocks = BlockChainBuilder::Blocks;

		// region utils

		using PropertyStateHashes = std::vector<std::pair<Hash256, Hash256>>;

		Hash256 GetComponentStateHash(const test::PeerLocalNodeTestContext& context) {
			auto subCacheMerkleRoots = context.localNode().cache().createView().calculateStateHash().SubCacheMerkleRoots;
			return subCacheMerkleRoots.empty() ? Hash256() : subCacheMerkleRoots[4]; // { Config, AccountState, Namespace, Mosaic, *Property* }
		}

		void AssertPropertyCount(const local::LocalNode& localNode, size_t numExpectedProperties) {
			// Assert:
			auto numProperties = test::GetCounterValue(localNode.counters(), "PROPERTY C");
			EXPECT_EQ(numExpectedProperties, numProperties);
		}

		template<typename TTestContext>
		std::pair<BlockChainBuilder, std::shared_ptr<model::Block>> PrepareProperty(
				TTestContext& context,
				const test::Accounts& accounts,
				test::StateHashCalculator& stateHashCalculator,
				PropertyStateHashes& stateHashes) {
			// Arrange:
			test::WaitForBoot(context);
			stateHashes.emplace_back(GetStateHash(context), GetComponentStateHash(context));

			// - prepare property
			test::PropertyTransactionsBuilder transactionsBuilder(accounts);
			transactionsBuilder.addTransfer(0, 2, Amount(1'000'000));
			transactionsBuilder.addTransfer(0, 3, Amount(1'000'000));
			transactionsBuilder.addAddressBlockProperty(2, 3);

			BlockChainBuilder builder(accounts, stateHashCalculator);
			auto pPropertyBlock = utils::UniqueToShared(builder.asSingleBlock(transactionsBuilder));

			// Act:
			test::ExternalSourceConnection connection;
			auto pIo = test::PushEntity(connection, ionet::PacketType::Push_Block, pPropertyBlock);

			// - wait for the chain height to change and for all height readers to disconnect
			test::WaitForHeightAndElements(context, Height(2), 1, 1);
			stateHashes.emplace_back(GetStateHash(context), GetComponentStateHash(context));

			// Assert: the cache has expected number of properties
			AssertPropertyCount(context.localNode(), 1);

			return std::make_pair(builder, pPropertyBlock);
		}

		// endregion

		// region TestFacade

		template<typename TTestContext, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions>
		class TestFacade {
		public:
			explicit TestFacade(TTestContext& context)
					: m_context(context)
					, m_accounts(4, TRemainingAccountVersions, TDefaultAccountVersion)
			{}

		public:
			const auto& accounts() const {
				return m_accounts;
			}

			const auto& stateHashes() const {
				return m_stateHashes;
			}

		public:
			void pushProperty() {
				// Arrange: push a property block
				auto stateHashCalculator = m_context.createStateHashCalculator();
				auto builderBlockPair = PrepareProperty(m_context, m_accounts, stateHashCalculator, m_stateHashes);

				// Sanity: all properties are still present
				AssertPropertyCount(m_context.localNode(), 1);

				m_allBlocks.emplace_back(builderBlockPair.second);
				m_pActiveBuilder = std::make_unique<BlockChainBuilder>(builderBlockPair.first);
			}

			Blocks createTailBlocks(utils::TimeSpan blockInterval, const consumer<test::PropertyTransactionsBuilder&>& addToBuilder) {
				auto stateHashCalculator = m_context.createStateHashCalculator();
				test::SeedStateHashCalculator(stateHashCalculator, m_allBlocks);

				test::PropertyTransactionsBuilder transactionsBuilder(m_accounts);
				addToBuilder(transactionsBuilder);

				auto builder = m_pActiveBuilder->createChainedBuilder(stateHashCalculator);
				builder.setBlockTimeInterval(blockInterval);
				return builder.asBlockChain(transactionsBuilder);
			}

		private:
			TTestContext& m_context;

			test::Accounts m_accounts;
			PropertyStateHashes m_stateHashes;
			std::unique_ptr<BlockChainBuilder> m_pActiveBuilder;
			std::vector<std::shared_ptr<model::Block>> m_allBlocks;
		};

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
		struct LocalNodeSyncPropertyIntegrityTests : public ::testing::Test {};
	}

	TYPED_TEST_CASE(LocalNodeSyncPropertyIntegrityTests, test_types);

	// region property (add)


	namespace {
		template<typename TTestContext, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions>
		PropertyStateHashes RunAddPropertyTest(TTestContext& context) {
			// Arrange:
			PropertyStateHashes stateHashes;
			test::Accounts accounts(4, TRemainingAccountVersions, TDefaultAccountVersion);
			auto stateHashCalculator = context.createStateHashCalculator();

			// Act + Assert:
			PrepareProperty(context, accounts, stateHashCalculator, stateHashes);

			return stateHashes;
		}
	}

	NO_STRESS_TYPED_TEST(LocalNodeSyncPropertyIntegrityTests, CanAddProperty) {
		// Arrange:
		test::StateHashDisabledTestContext context(test::NonNemesisTransactionPlugins::Property);

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunAddPropertyTest<test::StateHashDisabledTestContext, TypeParam::first_type::value, TypeParam::second_type::value>(context));

		// Assert:
		test::AssertAllZero(stateHashesPair, 2);
	}

	NO_STRESS_TYPED_TEST(LocalNodeSyncPropertyIntegrityTests, CanAddPropertyWithStateHashEnabled) {
		// Arrange:
		test::StateHashEnabledTestContext context(test::NonNemesisTransactionPlugins::Property);

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunAddPropertyTest<test::StateHashEnabledTestContext, TypeParam::first_type::value, TypeParam::second_type::value>(context));

		// Assert: all state hashes are nonzero
		test::AssertAllNonZero(stateHashesPair.first, 2);
		test::AssertUnique(stateHashesPair.first);

		// - property cache merkle root is only nonzero when property is added
		ASSERT_EQ(2u, stateHashesPair.second.size());
		EXPECT_EQ(Hash256(), stateHashesPair.second[0]);
		EXPECT_NE(Hash256(), stateHashesPair.second[1]);
	}

	// endregion

	// region property (remove)

	namespace {
		template<typename TTestContext, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions>
		PropertyStateHashes RunRemovePropertyTest(TTestContext& context) {
			// Arrange: create a property
			TestFacade<TTestContext, TDefaultAccountVersion, TRemainingAccountVersions> facade(context);
			facade.pushProperty();

			// - prepare block that will remove the property
			auto nextBlocks = facade.createTailBlocks(utils::TimeSpan::FromSeconds(60), [](auto& transactionsBuilder) {
				transactionsBuilder.addAddressUnblockProperty(2, 3);
			});

			// Act:
			test::ExternalSourceConnection connection;
			auto pIo1 = test::PushEntities(connection, ionet::PacketType::Push_Block, nextBlocks);

			// - wait for the chain height to change and for all height readers to disconnect
			test::WaitForHeightAndElements(context, Height(3), 2, 1);

			// Assert: the cache has no properties
			AssertPropertyCount(context.localNode(), 0);

			auto stateHashes = facade.stateHashes();
			stateHashes.emplace_back(GetStateHash(context), GetComponentStateHash(context));
			return stateHashes;
		}
	}

	NO_STRESS_TYPED_TEST(LocalNodeSyncPropertyIntegrityTests, CanRemoveProperty) {
		// Arrange:
		test::StateHashDisabledTestContext context(test::NonNemesisTransactionPlugins::Property);

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunRemovePropertyTest<test::StateHashDisabledTestContext, TypeParam::first_type::value, TypeParam::second_type::value>(context));

		// Assert:
		test::AssertAllZero(stateHashesPair, 3);
	}

	NO_STRESS_TYPED_TEST(LocalNodeSyncPropertyIntegrityTests, CanRemovePropertyWithStateHashEnabled) {
		// Arrange:
		test::StateHashEnabledTestContext context(test::NonNemesisTransactionPlugins::Property);

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunRemovePropertyTest<test::StateHashEnabledTestContext, TypeParam::first_type::value, TypeParam::second_type::value>(context));

		// Assert: all state hashes are nonzero (since importance is recalculated every block none of the hashes are the same)
		test::AssertAllNonZero(stateHashesPair.first, 3);
		test::AssertUnique(stateHashesPair.first);

		// - property cache merkle root is initially zero (no properties in nemesis)
		ASSERT_EQ(3u, stateHashesPair.second.size());
		EXPECT_EQ(Hash256(), stateHashesPair.second[0]);

		// - property is active at 1
		EXPECT_NE(stateHashesPair.second[0], stateHashesPair.second[1]);

		// - property is finally removed
		EXPECT_EQ(stateHashesPair.second[0], stateHashesPair.second[2]);
	}

	// endregion

	// region property (add + remove) [single chain part]

	namespace {
		template<typename TTestContext, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions>
		PropertyStateHashes RunAddAndRemovePropertyTest(TTestContext& context) {
			// Arrange:
			PropertyStateHashes stateHashes;
			test::Accounts accounts(4, TRemainingAccountVersions, TDefaultAccountVersion);
			auto stateHashCalculator = context.createStateHashCalculator();

			// - wait for boot
			test::WaitForBoot(context);
			stateHashes.emplace_back(GetStateHash(context), GetComponentStateHash(context));

			// - prepare property chain with add and remove
			test::PropertyTransactionsBuilder transactionsBuilder(accounts);
			transactionsBuilder.addTransfer(0, 2, Amount(1'000'000));
			transactionsBuilder.addTransfer(0, 3, Amount(1'000'000));
			transactionsBuilder.addAddressBlockProperty(2, 3);
			transactionsBuilder.addAddressUnblockProperty(2, 3);

			// - send chain
			BlockChainBuilder builder(accounts, stateHashCalculator);
			auto blocks = builder.asBlockChain(transactionsBuilder);

			test::ExternalSourceConnection connection;
			test::PushEntities(connection, ionet::PacketType::Push_Block, blocks);
			test::WaitForHeightAndElements(context, Height(5), 1, 1);
			stateHashes.emplace_back(GetStateHash(context), GetComponentStateHash(context));

			// Assert: the cache has no properties
			AssertPropertyCount(context.localNode(), 0);

			return stateHashes;
		}
	}

	NO_STRESS_TYPED_TEST(LocalNodeSyncPropertyIntegrityTests, CanAddAndRemoveProperty_SingleChainPart) {
		// Arrange:
		test::StateHashDisabledTestContext context(test::NonNemesisTransactionPlugins::Property);

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunAddAndRemovePropertyTest<test::StateHashDisabledTestContext, TypeParam::first_type::value, TypeParam::second_type::value>(context));

		// Assert:
		test::AssertAllZero(stateHashesPair, 2);
	}

	NO_STRESS_TYPED_TEST(LocalNodeSyncPropertyIntegrityTests, CanAddAndRemovePropertyWithStateHashEnabled_SingleChainPart) {
		// Arrange:
		test::StateHashEnabledTestContext context(test::NonNemesisTransactionPlugins::Property);

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunAddAndRemovePropertyTest<test::StateHashEnabledTestContext, TypeParam::first_type::value, TypeParam::second_type::value>(context));

		// Assert: all state hashes are nonzero (since importance is recalculated every block none of the hashes are the same)
		test::AssertAllNonZero(stateHashesPair.first, 2);
		test::AssertUnique(stateHashesPair.first);

		// - all property merkle roots are zero because property is only active inbetween
		test::AssertAllZero(stateHashesPair.second, 2);
	}

	// endregion
}}
