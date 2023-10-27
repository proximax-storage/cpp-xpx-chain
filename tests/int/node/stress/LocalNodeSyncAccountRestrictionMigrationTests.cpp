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

#include <tests/int/node/stress/test/TransactionBuilderNetworkConfigCapability.h>
#include <boost/algorithm/string/replace.hpp>
#include "tests/int/node/stress/test/ExpiryTestUtils.h"
#include "tests/int/node/stress/test/LocalNodeSyncIntegrityTestUtils.h"
#include "tests/int/node/stress/test/TransactionsBuilder.h"
#include "tests/int/node/stress/test/TransactionBuilderTransferCapability.h"
#include "tests/int/node/stress/test/TransactionBuilderPropertyCapability.h"
#include "tests/TestHarness.h"
#include "tests/test/nodeps/MijinConstants.h"
#include "tests/test/nodeps/data/BasicExtendedNemesisMemoryBlockStorage_data.h"
#include "src/catapult/extensions/NemesisBlockLoader.h"
#include "plugins/txes/restriction_account/src/cache/AccountRestrictionCache.h"

namespace catapult { namespace local {

#define TEST_CLASS LocalNodeSyncAccountRestrictionMigrationTests

	namespace {
		using BlockChainBuilder = test::BlockChainBuilder;
		using Blocks = BlockChainBuilder::Blocks;

		// region utils

		using PropertyStateHashes = std::vector<std::pair<Hash256, Hash256>>;

		Hash256 GetComponentStateHash(const test::PeerLocalNodeTestContext& context) {
			auto subCacheMerkleRoots = context.localNode().cache().createView().calculateStateHash().SubCacheMerkleRoots;
			return subCacheMerkleRoots.empty() ? Hash256() : subCacheMerkleRoots[5]; // { // config, accountstate, locksecret, block diff, namespace, mosaic, secret lock, property, upgrade, levy, lockfund,globalstore }
		}

		void AssertPropertyCount(const local::LocalNode& localNode, size_t numExpectedProperties) {
			// Assert:
			auto numProperties = test::GetCounterValue(localNode.counters(), "PROPERTY C");
			EXPECT_EQ(numExpectedProperties, numProperties);
		}
		template<typename TTestContext>
		std::pair<BlockChainBuilder, std::shared_ptr<model::Block>> GenerateNetworkUpgrade(const TTestContext& context,
									const test::Accounts& accounts,
									BlockChainBuilder& builder,
									test::ExternalSourceConnection& connection)
		{

			test::TransactionsBuilder transactionsBuilder(accounts);
			auto networkConfigBuilder = transactionsBuilder.template getCapability<test::TransactionBuilderNetworkConfigCapability>();
			mocks::MockMemoryBlockStorage storage([](){return mocks::CreateNemesisBlockElement(test::Extended_Basic_MemoryBlockStorage_NemesisBlockData);});
			auto pNemesisBlockElement = storage.loadBlockElement(Height(1));
			auto configs = extensions::NemesisBlockLoader::ReadNetworkConfigurationAsStrings(pNemesisBlockElement);
			std::string supportedEntities = std::get<1>(configs);
			std::string content  = std::get<0>(configs);
			boost::algorithm::replace_first(content, "[plugin:catapult.plugins.restrictionaccount]\n\nenabled = false", "[plugin:catapult.plugins.restrictionaccount]\n\nenabled = true");
			networkConfigBuilder->addNetworkConfigUpdate(content, supportedEntities, BlockDuration(1));
			auto pUpgradeBlock = utils::UniqueToShared(builder.asSingleBlock(transactionsBuilder));
			test::PushEntity(connection, ionet::PacketType::Push_Block, pUpgradeBlock);
			test::WaitForHeightAndElements(context, Height(3), 2, 1);
			return std::make_pair(builder, pUpgradeBlock);
		}

		template<typename TTestContext>
		std::pair<BlockChainBuilder, std::shared_ptr<model::Block>> ProcessEmptyBlock(const TTestContext& context,
																						   const test::Accounts& accounts,
																						   BlockChainBuilder& builder,
																						   test::ExternalSourceConnection& connection)
		{

			test::TransactionsBuilder transactionsBuilder(accounts);
			auto pEmptyBlock = utils::UniqueToShared(builder.asSingleBlock(transactionsBuilder));
			test::PushEntity(connection, ionet::PacketType::Push_Block, pEmptyBlock);
			test::WaitForHeightAndElements(context, Height(4), 3, 1);
			return std::make_pair(builder, pEmptyBlock);
		}

		template<typename TTestContext>
		std::pair<BlockChainBuilder, std::vector<std::shared_ptr<model::Block>>> PrepareProperties(
				TTestContext& context,
				const test::Accounts& accounts,
				test::StateHashCalculator& stateHashCalculator,
				PropertyStateHashes& stateHashes) {
			// Arrange:
			test::WaitForBoot(context);
			stateHashes.emplace_back(GetStateHash(context), GetComponentStateHash(context));


			// - prepare property
			test::TransactionsBuilder transactionsBuilder(accounts);
			auto transferBuilder = transactionsBuilder.template getCapability<test::TransactionBuilderTransferCapability>();
			auto propertyBuilder = transactionsBuilder.template getCapability<test::TransactionBuilderPropertyCapability>();

			transferBuilder->addTransfer(0, 2, Amount(1'000'000));
			transferBuilder->addTransfer(0, 3, Amount(1'000'000));
			transferBuilder->addTransfer(0, 4, Amount(1'000'000));
			transferBuilder->addTransfer(0, 5, Amount(1'000'000));
			transferBuilder->addTransfer(0, 6, Amount(1'000'000));
			transferBuilder->addTransfer(0, 7, Amount(1'000'000));
			transferBuilder->addTransfer(0, 8, Amount(1'000'000));
			transferBuilder->addTransfer(0, 9, Amount(1'000'000));
			propertyBuilder->addAddressBlockProperty(2, 3);
			propertyBuilder->addAddressBlockProperty(3, 4);
			propertyBuilder->addAddressBlockProperty(4, 5);
			propertyBuilder->addAddressBlockProperty(5, 6);
			propertyBuilder->addAddressBlockProperty(6, 7);
			propertyBuilder->addAddressBlockProperty(7, 8);
			propertyBuilder->addAddressBlockProperty(8, 9);

			auto& accountStateCache = context.localNode().cache().template sub<cache::AccountStateCache>();

			BlockChainBuilder builder(accounts, stateHashCalculator, context.configHolder(), &accountStateCache, context.dataDirectory());


			// Act:
			test::ExternalSourceConnection connection;
			std::vector<std::shared_ptr<model::Block>> entities;

			auto pPropertyBlock = utils::UniqueToShared(builder.asSingleBlock(transactionsBuilder));
			auto pIo = test::PushEntity(connection, ionet::PacketType::Push_Block, pPropertyBlock);

			// - wait for the chain height to change and for all height readers to disconnect
			test::WaitForHeightAndElements(context, Height(2), 1, 1);
			stateHashes.emplace_back(GetStateHash(context), GetComponentStateHash(context));

			// Assert: the cache has expected number of properties
			AssertPropertyCount(context.localNode(), 7);
			entities.push_back(pPropertyBlock);
			return std::make_pair(builder, entities);
		}

		// endregion

		// region TestFacade

		template<typename TTestContext, uint32_t TDefaultAccountVersion, uint32_t TRemainingAccountVersions>
		class TestFacade {
		public:
			explicit TestFacade(TTestContext& context)
					: m_context(context)
					, m_accounts(crypto::KeyPair::FromString(test::Mijin_Test_Nemesis_Private_Key, 1), 4, TRemainingAccountVersions, TDefaultAccountVersion)
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

				for(auto block : builderBlockPair.second)
					m_allBlocks.emplace_back(block);
				m_pActiveBuilder = std::make_unique<BlockChainBuilder>(builderBlockPair.first);
			}

			Blocks createTailBlocks(utils::TimeSpan blockInterval, const consumer<test::TransactionBuilderPropertyCapability&>& addToBuilder) {
				auto stateHashCalculator = m_context.createStateHashCalculator();
				test::SeedStateHashCalculator(stateHashCalculator, m_allBlocks, test::Extended_Basic_MemoryBlockStorage_NemesisBlockData);

				test::TransactionsBuilder transactionsBuilder(m_accounts);
				auto propertyBuilder = transactionsBuilder.getCapability<test::TransactionBuilderPropertyCapability>();
				addToBuilder(*propertyBuilder);

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
		template<typename TTestContext>
		PropertyStateHashes RunMigratePropertiesTest(TTestContext& context, const test::Accounts& accounts) {
			// Arrange:
			PropertyStateHashes stateHashes;

			auto stateHashCalculator = context.createStateHashCalculator();

			// Act + Assert:
			auto result = PrepareProperties(context, accounts, stateHashCalculator, stateHashes);

			test::ExternalSourceConnection connection;

			GenerateNetworkUpgrade(context, accounts, result.first, connection);
			ProcessEmptyBlock(context, accounts, result.first, connection);

			return stateHashes;
		}

	}

	TEST(TEST_CLASS, CanMigratePropertiesToAccountRestrictions) {
		// Arrange:
		test::Accounts accounts(crypto::KeyPair::FromString(test::Mijin_Test_Nemesis_Private_Key, 1), 10, 1, 1);
		test::StateHashDisabledTestContext context([](config::BlockchainConfiguration& config ) {
			const_cast<config::NodeConfiguration&>(config.Node).ShouldUseCacheDatabaseStorage = true;
		});

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunMigratePropertiesTest<test::StateHashDisabledTestContext>(context, accounts));

		// Assert:
		test::AssertAllZero(stateHashesPair, 2);
		auto catapultCache = context.localNode().cache().createView();
		auto& accountRestrictionCache = catapultCache.sub<cache::AccountRestrictionCache>();
		for(size_t i = 2; i < 9; i++) {
			auto restrictionBaseIter = accountRestrictionCache.find(accounts.getAddress(i));
			auto restrictionBase = restrictionBaseIter.get();
			auto targetAddress = accounts.getAddress(i+1);
			auto vectorAddress = std::vector<uint8_t>(targetAddress.cbegin(), targetAddress.cend());
			auto restrictions = restrictionBase.restriction(model::AccountRestrictionFlags::Block | model::AccountRestrictionFlags::Address);
			ASSERT_EQ(restrictionBase.address(), accounts.getAddress(i));
			ASSERT_TRUE(restrictions.contains(vectorAddress));
		}
	}
	// endregion
}}
