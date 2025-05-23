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
#include "tests/int/node/stress/test/SecretLockTransactionsBuilder.h"

namespace catapult { namespace local {

#define TEST_CLASS LocalNodeSyncSecretLockIntegrityTests

	namespace {
		using BlockChainBuilder = test::BlockChainBuilder;
		using Blocks = BlockChainBuilder::Blocks;

		constexpr auto Lock_Duration = BlockDuration(10);

		// region utils

		using SecretLockStateHashes = std::vector<std::pair<Hash256, Hash256>>;

		Hash256 GetComponentStateHash(const test::PeerLocalNodeTestContext& context, size_t index) {
			auto subCacheMerkleRoots = context.localNode().cache().createView().calculateStateHash().SubCacheMerkleRoots;
			return subCacheMerkleRoots.empty() ? Hash256() : subCacheMerkleRoots[index];
		}

		Hash256 GetComponentStateHash(const test::PeerLocalNodeTestContext& context) {
			return GetComponentStateHash(context, 7); // { Config, AccountState, Namespace, Metadata, Mosaic, Multisig, HashLock, *SecretLock* }
		}

		struct SecretLockTuple {
			BlockChainBuilder Builder;
			std::shared_ptr<model::Block> pSecretLockBlock;
			std::vector<uint8_t> Proof;
		};

		template<typename TTestContext>
		SecretLockTuple PrepareSecretLock(
				TTestContext& context,
				const test::Accounts& accounts,
				test::StateHashCalculator& stateHashCalculator,
				SecretLockStateHashes& stateHashes) {
			// Arrange:
			test::WaitForBoot(context);
			stateHashes.emplace_back(GetStateHash(context), GetComponentStateHash(context));

			// - prepare secret lock
			test::SecretLockTransactionsBuilder transactionsBuilder(accounts);
			transactionsBuilder.addTransfer(0, 2, Amount(1'000'000));
			transactionsBuilder.addTransfer(0, 3, Amount(1'000'000));
			auto secretProof = transactionsBuilder.addSecretLock(2, 3, Amount(100'000), Lock_Duration);

			BlockChainBuilder builder(accounts, stateHashCalculator);
			auto pSecretLockBlock = utils::UniqueToShared(builder.asSingleBlock(transactionsBuilder));

			// Act:
			test::ExternalSourceConnection connection;
			auto pIo = test::PushEntity(connection, ionet::PacketType::Push_Block, pSecretLockBlock);

			// - wait for the chain height to change and for all height readers to disconnect
			test::WaitForHeightAndElements(context, Height(2), 1, 1);
			stateHashes.emplace_back(GetStateHash(context), GetComponentStateHash(context));

			// Assert: the cache has expected balances
			test::AssertCurrencyBalances(accounts, context.localNode().cache(), {
				{ 2, Amount(900'000) },
				{ 3, Amount(1'000'000) }
			});

			return { builder, pSecretLockBlock, secretProof };
		}

		void AssertDeactivatingSecretLock(const std::pair<std::vector<Hash256>, std::vector<Hash256>>& stateHashesPair) {
			// Assert: all state hashes are nonzero (since importance is recalculated every block none of the hashes are the same)
			test::AssertAllNonZero(stateHashesPair.first, 4);
			test::AssertUnique(stateHashesPair.first);

			// - secret lock cache merkle root is initially zero (no locks in nemesis)
			ASSERT_EQ(4u, stateHashesPair.second.size());
			EXPECT_EQ(Hash256(), stateHashesPair.second[0]);

			// - secret lock is active at 1 and 2
			EXPECT_NE(stateHashesPair.second[0], stateHashesPair.second[1]);
			EXPECT_EQ(stateHashesPair.second[1], stateHashesPair.second[2]);

			// - secret lock is finally deactivated
			EXPECT_EQ(stateHashesPair.second[0], stateHashesPair.second[3]);
		}

		// endregion

		// region TestFacade

		template<typename TTestContext>
		class TestFacade {
		public:
			explicit TestFacade(TTestContext& context)
					: m_context(context)
					, m_accounts(4)
			{}

		public:
			const auto& accounts() const {
				return m_accounts;
			}

			const auto& stateHashes() const {
				return m_stateHashes;
			}

			auto numAliveChains() const {
				return m_numAliveChains;
			}

		public:
			std::vector<uint8_t> pushSecretLockAndTransferBlocks(uint32_t numBlocks) {
				// Arrange: push a secret lock block
				auto stateHashCalculator = m_context.createStateHashCalculator();
				auto secretLockTuple = PrepareSecretLock(m_context, m_accounts, stateHashCalculator, m_stateHashes);

				// - add the specified number of blocks
				test::ExternalSourceConnection connection;
				auto builder2 = secretLockTuple.Builder.createChainedBuilder();
				auto transferBlocksResult = PushTransferBlocks(m_context, connection, m_accounts, builder2, numBlocks);
				m_numAliveChains = transferBlocksResult.NumAliveChains;
				m_stateHashes.emplace_back(GetStateHash(m_context), GetComponentStateHash(m_context));

				// Sanity: all secret locks are still present
				test::AssertCurrencyBalances(m_accounts, m_context.localNode().cache(), {
					{ 2, Amount(900'000) },
					{ 3, Amount(1'000'000) }
				});

				m_allBlocks.emplace_back(secretLockTuple.pSecretLockBlock);
				m_allBlocks.insert(m_allBlocks.end(), transferBlocksResult.AllBlocks.cbegin(), transferBlocksResult.AllBlocks.cend());
				m_pActiveBuilder = std::make_unique<BlockChainBuilder>(builder2);
				return secretLockTuple.Proof;
			}

			Blocks createTailBlocks(utils::TimeSpan blockInterval, const consumer<test::SecretLockTransactionsBuilder&>& addToBuilder) {
				auto stateHashCalculator = m_context.createStateHashCalculator();
				test::SeedStateHashCalculator(stateHashCalculator, m_allBlocks);

				test::SecretLockTransactionsBuilder transactionsBuilder(m_accounts);
				addToBuilder(transactionsBuilder);

				auto builder = m_pActiveBuilder->createChainedBuilder(stateHashCalculator);
				builder.setBlockTimeInterval(blockInterval);
				return builder.asBlockChain(transactionsBuilder);
			}

		private:
			TTestContext& m_context;

			test::Accounts m_accounts;
			SecretLockStateHashes m_stateHashes;
			std::unique_ptr<BlockChainBuilder> m_pActiveBuilder;
			std::vector<std::shared_ptr<model::Block>> m_allBlocks;
			uint32_t m_numAliveChains;
		};

		// endregion
	}

	// region secret lock (register)

	namespace {
		template<typename TTestContext>
		SecretLockStateHashes RunLockSecretLockTest(TTestContext& context) {
			// Arrange:
			SecretLockStateHashes stateHashes;
			test::Accounts accounts(4);
			auto stateHashCalculator = context.createStateHashCalculator();

			// Act + Assert:
			PrepareSecretLock(context, accounts, stateHashCalculator, stateHashes);

			return stateHashes;
		}
	}

	NO_STRESS_TEST(TEST_CLASS, CanLockSecretLock) {
		// Arrange:
		test::StateHashDisabledTestContext context(test::NonNemesisTransactionPlugins::Lock_Secret, [](auto& config) {
			const_cast<config::NodeConfiguration&>(config.Node).TransactionBatchSize = 50;
		});

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunLockSecretLockTest(context));

		// Assert:
		test::AssertAllZero(stateHashesPair, 2);
	}

	NO_STRESS_TEST(TEST_CLASS, CanLockSecretLockWithStateHashEnabled) {
		// Arrange:
		test::StateHashEnabledTestContext context(test::NonNemesisTransactionPlugins::Lock_Secret, [](auto& config) {
			const_cast<config::NodeConfiguration&>(config.Node).TransactionBatchSize = 50;
		});

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunLockSecretLockTest(context));

		// Assert: all state hashes are nonzero
		test::AssertAllNonZero(stateHashesPair.first, 2);
		test::AssertUnique(stateHashesPair.first);

		// - secret lock cache merkle root is only nonzero when lock is active
		ASSERT_EQ(2u, stateHashesPair.second.size());
		EXPECT_EQ(Hash256(), stateHashesPair.second[0]);
		EXPECT_NE(Hash256(), stateHashesPair.second[1]);
	}

	// endregion

	// region secret lock (unlock)

	namespace {
		template<typename TTestContext>
		SecretLockStateHashes RunUnlockSecretLockTest(TTestContext& context) {
			// Arrange: create a secret lock followed by empty blocks so that two more blocks will trigger expiry
			TestFacade facade(context);
			auto numEmptyBlocks = static_cast<uint32_t>(Lock_Duration.unwrap() - 2);
			auto secretProof = facade.pushSecretLockAndTransferBlocks(numEmptyBlocks);

			// - prepare blocks that will unlock the secret
			auto nextBlocks = facade.createTailBlocks(utils::TimeSpan::FromSeconds(15), [&secretProof](auto& transactionsBuilder) {
				transactionsBuilder.addSecretProof(2, 3, secretProof);
			});

			// Act:
			test::ExternalSourceConnection connection;
			auto pIo1 = test::PushEntities(connection, ionet::PacketType::Push_Block, nextBlocks);

			// - wait for the chain height to change and for all height readers to disconnect
			test::WaitForHeightAndElements(context, Height(2 + numEmptyBlocks + 1), 1 + facade.numAliveChains() + 1, 1);

			// Assert: the cache has the expected balances and the lock has been unlocked
			test::AssertCurrencyBalances(facade.accounts(), context.localNode().cache(), {
				{ 2, Amount(900'000) },
				{ 3, Amount(1'100'000) }
			});

			auto stateHashes = facade.stateHashes();
			stateHashes.emplace_back(GetStateHash(context), GetComponentStateHash(context));
			return stateHashes;
		}
	}

	NO_STRESS_TEST(TEST_CLASS, CanUnlockSecretLock) {
		// Arrange:
		test::StateHashDisabledTestContext context(test::NonNemesisTransactionPlugins::Lock_Secret, [](auto& config) {
			const_cast<config::NodeConfiguration&>(config.Node).TransactionBatchSize = 50;
		});

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunUnlockSecretLockTest(context));

		// Assert:
		test::AssertAllZero(stateHashesPair, 4);
	}

	NO_STRESS_TEST(TEST_CLASS, CanUnlockSecretLockWithStateHashEnabled) {
		// Arrange:
		test::StateHashEnabledTestContext context(test::NonNemesisTransactionPlugins::Lock_Secret, [](auto& config) {
			const_cast<config::NodeConfiguration&>(config.Node).TransactionBatchSize = 50;
		});

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunUnlockSecretLockTest(context));

		// Assert:
		AssertDeactivatingSecretLock(stateHashesPair);
	}

	// endregion

	// region secret lock (expire)

	namespace {
		template<typename TTestContext>
		SecretLockStateHashes RunExpireSecretLockTest(TTestContext& context) {
			// Arrange: create a secret lock followed by empty blocks so that the next block will trigger expiry
			TestFacade facade(context);
			auto numEmptyBlocks = static_cast<uint32_t>(Lock_Duration.unwrap() - 1);
			facade.pushSecretLockAndTransferBlocks(numEmptyBlocks);

			// - prepare blocks that will cause the secret to expire
			auto nextBlocks = facade.createTailBlocks(utils::TimeSpan::FromSeconds(15), [](auto& transactionsBuilder) {
				transactionsBuilder.addTransfer(0, 1, Amount(1));
			});

			// Act:
			test::ExternalSourceConnection connection;
			auto pIo1 = test::PushEntities(connection, ionet::PacketType::Push_Block, nextBlocks);

			// - wait for the chain height to change and for all height readers to disconnect
			test::WaitForHeightAndElements(context, Height(2 + numEmptyBlocks + 1), 1 + facade.numAliveChains() + 1, 1);

			// Assert: the cache has the expected balances and the lock has expired
			test::AssertCurrencyBalances(facade.accounts(), context.localNode().cache(), {
				{ 2, Amount(1'000'000) },
				{ 3, Amount(1'000'000) }
			});

			auto stateHashes = facade.stateHashes();
			stateHashes.emplace_back(GetStateHash(context), GetComponentStateHash(context));
			return stateHashes;
		}
	}

	NO_STRESS_TEST(TEST_CLASS, CanExpireSecretLock) {
		// Arrange:
		test::StateHashDisabledTestContext context(test::NonNemesisTransactionPlugins::Lock_Secret, [](auto& config) {
			const_cast<config::NodeConfiguration&>(config.Node).TransactionBatchSize = 50;
		});

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunExpireSecretLockTest(context));

		// Assert:
		test::AssertAllZero(stateHashesPair, 4);
	}

	NO_STRESS_TEST(TEST_CLASS, CanExpireSecretLockWithStateHashEnabled) {
		// Arrange:
		test::StateHashEnabledTestContext context(test::NonNemesisTransactionPlugins::Lock_Secret, [](auto& config) {
			const_cast<config::NodeConfiguration&>(config.Node).TransactionBatchSize = 50;
		});

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunExpireSecretLockTest(context));

		// Assert:
		AssertDeactivatingSecretLock(stateHashesPair);
	}

	// endregion

	// region secret lock (unlock + rollback)

	namespace {
		template<typename TTestContext>
		SecretLockStateHashes RunUnlockAndRollbackSecretLockTest(TTestContext& context) {
			// Arrange: create a secret lock followed by empty blocks so that three more blocks will trigger expiry
			TestFacade facade(context);
			auto numEmptyBlocks = static_cast<uint32_t>(Lock_Duration.unwrap() - 3);
			auto secretProof = facade.pushSecretLockAndTransferBlocks(numEmptyBlocks);

			// - prepare two sets of blocks one of which will unlock secret (better block time will yield better chain)
			auto worseBlocks = facade.createTailBlocks(utils::TimeSpan::FromSeconds(15), [&secretProof](auto& transactionsBuilder) {
				transactionsBuilder.addSecretProof(2, 3, secretProof);
			});
			auto betterBlocks = facade.createTailBlocks(utils::TimeSpan::FromSeconds(12), [](auto& transactionsBuilder) {
				transactionsBuilder.addTransfer(0, 1, Amount(1));
				transactionsBuilder.addTransfer(0, 1, Amount(1));
			});

			// Act:
			test::ExternalSourceConnection connection;
			auto pIo1 = test::PushEntities(connection, ionet::PacketType::Push_Block, worseBlocks);
			auto pIo2 = test::PushEntities(connection, ionet::PacketType::Push_Block, betterBlocks);

			// - wait for the chain height to change and for all height readers to disconnect
			test::WaitForHeightAndElements(context, Height(2 + numEmptyBlocks + 2), 1 + facade.numAliveChains() + 2, 2);

			// Assert: the cache has the expected balances and the lock is still set
			test::AssertCurrencyBalances(facade.accounts(), context.localNode().cache(), {
				{ 1, Amount(numEmptyBlocks + 2) },
				{ 2, Amount(900'000) },
				{ 3, Amount(1'000'000) }
			});

			auto stateHashes = facade.stateHashes();
			stateHashes.emplace_back(GetStateHash(context), GetComponentStateHash(context));
			return stateHashes;
		}
	}

	NO_STRESS_TEST(TEST_CLASS, CanUnlockAndRollbackSecretLock) {
		// Arrange:
		test::StateHashDisabledTestContext context(test::NonNemesisTransactionPlugins::Lock_Secret, [](auto& config) {
			const_cast<config::NodeConfiguration&>(config.Node).TransactionBatchSize = 50;
		});

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunUnlockAndRollbackSecretLockTest(context));

		// Assert:
		test::AssertAllZero(stateHashesPair, 4);
	}

	NO_STRESS_TEST(TEST_CLASS, CanUnlockAndRollbackSecretLockWithStateHashEnabled) {
		// Arrange:
		test::StateHashEnabledTestContext context(test::NonNemesisTransactionPlugins::Lock_Secret, [](auto& config) {
			const_cast<config::NodeConfiguration&>(config.Node).TransactionBatchSize = 50;
		});

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunUnlockAndRollbackSecretLockTest(context));

		// Assert: all state hashes are nonzero (since importance is recalculated every block none of the hashes are the same)
		test::AssertAllNonZero(stateHashesPair.first, 4);
		test::AssertUnique(stateHashesPair.first);

		// - secret lock cache merkle root is initially zero (no locks in nemesis)
		ASSERT_EQ(4u, stateHashesPair.second.size());
		EXPECT_EQ(Hash256(), stateHashesPair.second[0]);

		// - secret lock is active at 1, 2 and 3
		EXPECT_NE(stateHashesPair.second[0], stateHashesPair.second[1]);
		EXPECT_EQ(stateHashesPair.second[1], stateHashesPair.second[2]);
		EXPECT_EQ(stateHashesPair.second[1], stateHashesPair.second[3]);
	}

	// endregion

	// region secret lock (expire + rollback)

	namespace {
		template<typename TTestContext>
		SecretLockStateHashes RunExpireAndRollbackSecretLockTest(TTestContext& context) {
			// Arrange: create a secret lock followed by empty blocks so that the next block will trigger expiry
			TestFacade facade(context);
			auto numEmptyBlocks = static_cast<uint32_t>(Lock_Duration.unwrap() - 1);
			facade.pushSecretLockAndTransferBlocks(numEmptyBlocks);

			// - prepare two sets of blocks that will trigger expiry (better block time will yield better chain)
			auto worseBlocks = facade.createTailBlocks(utils::TimeSpan::FromSeconds(15), [](auto& transactionsBuilder) {
				transactionsBuilder.addTransfer(0, 1, Amount(1));
			});
			auto betterBlocks = facade.createTailBlocks(utils::TimeSpan::FromSeconds(12), [](auto& transactionsBuilder) {
				transactionsBuilder.addTransfer(0, 1, Amount(1));
				transactionsBuilder.addTransfer(0, 1, Amount(1));
			});

			// Act:
			test::ExternalSourceConnection connection;
			auto pIo1 = test::PushEntities(connection, ionet::PacketType::Push_Block, worseBlocks);
			auto pIo2 = test::PushEntities(connection, ionet::PacketType::Push_Block, betterBlocks);

			// - wait for the chain height to change and for all height readers to disconnect
			test::WaitForHeightAndElements(context, Height(2 + numEmptyBlocks + 2), 1 + facade.numAliveChains() + 2, 2);

			// Assert: the cache has the expected balances and the lock has expired
			test::AssertCurrencyBalances(facade.accounts(), context.localNode().cache(), {
				{ 1, Amount(numEmptyBlocks + 2) },
				{ 2, Amount(1'000'000) },
				{ 3, Amount(1'000'000) }
			});

			auto stateHashes = facade.stateHashes();
			stateHashes.emplace_back(GetStateHash(context), GetComponentStateHash(context));
			return stateHashes;
		}
	}

	NO_STRESS_TEST(TEST_CLASS, CanExpireAndRollbackSecretLock) {
		// Arrange:
		test::StateHashDisabledTestContext context(test::NonNemesisTransactionPlugins::Lock_Secret, [](auto& config) {
			const_cast<config::NodeConfiguration&>(config.Node).TransactionBatchSize = 50;
		});

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunExpireAndRollbackSecretLockTest(context));

		// Assert:
		test::AssertAllZero(stateHashesPair, 4);
	}

	NO_STRESS_TEST(TEST_CLASS, CanExpireAndRollbackSecretLockWithStateHashEnabled) {
		// Arrange:
		test::StateHashEnabledTestContext context(test::NonNemesisTransactionPlugins::Lock_Secret, [](auto& config) {
			const_cast<config::NodeConfiguration&>(config.Node).TransactionBatchSize = 50;
		});

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunExpireAndRollbackSecretLockTest(context));

		// Assert:
		AssertDeactivatingSecretLock(stateHashesPair);
	}

	// endregion

	// region secret lock (add lock + rollback + readd lock at different height)

	namespace {
		template<typename TTestContext>
		class SecretLockRollbackTestContext {
		public:
			SecretLockRollbackTestContext()
					: m_context(test::NonNemesisTransactionPlugins::Lock_Secret, ConfigTransform)
					, m_accounts(4)
			{}

		public:
			SecretLockStateHashes runRegisterLockAndRollbackAndReregisterLockSecretLockTest() {
				// Arrange:
				SecretLockStateHashes stateHashes;
				BlockChainBuilder::Blocks allBlocks;

				// - wait for boot
				test::WaitForBoot(m_context);

				// - fund accounts
				auto builder = fundAccounts(allBlocks);

				// - add secret lock and then force rollback
				test::SecretLockTransactionsBuilder transactionsBuilder(m_accounts);
				auto secretProof = transactionsBuilder.addSecretLock(2, 3, Amount(100'000), Lock_Duration);
				auto worseBlocks = createBlocks(transactionsBuilder, builder, allBlocks);

				test::SecretLockTransactionsBuilder transactionsBuilder2(m_accounts);
				transactionsBuilder2.addTransfer(0, 1, Amount(1));

				{
					auto stateHashCalculator = m_context.createStateHashCalculator();
					test::SeedStateHashCalculator(stateHashCalculator, allBlocks);
					builder = builder.createChainedBuilder(stateHashCalculator);
					builder.setBlockTimeInterval(utils::TimeSpan::FromSeconds(14));
					auto betterBlocks = builder.asBlockChain(transactionsBuilder2);
					allBlocks.push_back(betterBlocks[0]);

					test::PushEntities(m_connection, ionet::PacketType::Push_Block, worseBlocks);
					test::PushEntities(m_connection, ionet::PacketType::Push_Block, betterBlocks);
					test::WaitForHeightAndElements(m_context, Height(4), 3, 1);
				}

				// Sanity: the cache has expected balances
				test::AssertCurrencyBalances(m_accounts, m_context.localNode().cache(), {
					{ 1, Amount(1) },
					{ 2, Amount(1'000'000) },
					{ 3, Amount(1'000'000) }
				});

				// - readd secret lock
				auto stateHashCalculator2 = m_context.createStateHashCalculator();
				test::SeedStateHashCalculator(stateHashCalculator2, allBlocks);
				auto builder3 = builder.createChainedBuilder(stateHashCalculator2);

				test::SecretLockTransactionsBuilder transactionsBuilder3(m_accounts);
				transactionsBuilder3.addSecretLock(2, 3, Amount(100'000), Lock_Duration, secretProof);
				auto blocks2 = builder3.asBlockChain(transactionsBuilder3);
				test::PushEntities(m_connection, ionet::PacketType::Push_Block, blocks2);
				test::WaitForHeightAndElements(m_context, Height(5), 4, 1);

				// - add empty blocks up to height where first added secret lock would expire next block
				for (auto i = 0u; i < 8; ++i)
					allBlocks.push_back(pushBlockAndWait(builder3, Height(6u + i)));

				stateHashes.emplace_back(GetStateHash(m_context), GetComponentStateHash(m_context, 1));

				// Act: add a single block, secret lock should not expire
				allBlocks.push_back(pushBlockAndWait(builder3, Height(14)));
				stateHashes.emplace_back(GetStateHash(m_context), GetComponentStateHash(m_context, 1));

				// - add another block, secret lock will expire
				allBlocks.push_back(pushBlockAndWait(builder3, Height(15)));
				stateHashes.emplace_back(GetStateHash(m_context), GetComponentStateHash(m_context, 1));

				return stateHashes;
			}

		private:
			static void ConfigTransform(config::BlockchainConfiguration& config) {
				// with importance grouping 1 the account state cache would change with every block, which is unwanted in the test
				const_cast<model::NetworkConfiguration&>(config.Network).ImportanceGrouping = 100;
				const_cast<config::NodeConfiguration&>(config.Node).TransactionBatchSize = 50;
			}

		private:
			BlockChainBuilder fundAccounts(BlockChainBuilder::Blocks& allBlocks) {
				auto stateHashCalculator = m_context.createStateHashCalculator();

				// - fund accounts
				test::SecretLockTransactionsBuilder transactionsBuilder(m_accounts);
				transactionsBuilder.addTransfer(0, 2, Amount(1'000'000));
				transactionsBuilder.addTransfer(0, 3, Amount(1'000'000));

				// - send chain
				BlockChainBuilder builder(m_accounts, stateHashCalculator);
				auto blocks = builder.asBlockChain(transactionsBuilder);
				allBlocks.insert(allBlocks.cend(), blocks.cbegin(), blocks.cend());

				test::PushEntities(m_connection, ionet::PacketType::Push_Block, blocks);
				test::WaitForHeightAndElements(m_context, Height(3), 1, 1);

				// Sanity: the cache has expected balances
				test::AssertCurrencyBalances(m_accounts, m_context.localNode().cache(), {
					{ 2, Amount(1'000'000) },
					{ 3, Amount(1'000'000) }
				});

				return builder;
			}

			BlockChainBuilder::Blocks createBlocks(
					test::SecretLockTransactionsBuilder& transactionGenerator,
					BlockChainBuilder& builder,
					BlockChainBuilder::Blocks& allBlocks,
					utils::TimeSpan blockTimeInterval = utils::TimeSpan::FromSeconds(15)) {
				auto stateHashCalculator = m_context.createStateHashCalculator();
				test::SeedStateHashCalculator(stateHashCalculator, allBlocks);
				auto tempBuilder = builder.createChainedBuilder(stateHashCalculator);
				tempBuilder.setBlockTimeInterval(blockTimeInterval);
				return tempBuilder.asBlockChain(transactionGenerator);
			}

			std::shared_ptr<model::Block> pushBlockAndWait(BlockChainBuilder& builder, Height height) {
				test::SecretLockTransactionsBuilder transactionsBuilder(m_accounts);
				auto pBlock = utils::UniqueToShared(builder.asSingleBlock(transactionsBuilder));
				test::PushEntity(m_connection, ionet::PacketType::Push_Block, pBlock);
				test::WaitForHeightAndElements(m_context, height, static_cast<uint32_t>(height.unwrap() - 1), 1);
				return pBlock;
			}

		private:
			TTestContext m_context;
			test::Accounts m_accounts;
			test::ExternalSourceConnection m_connection;
		};
	}

	NO_STRESS_TEST(TEST_CLASS, CanAddAndRollbackAndReaddSecretLock) {
		// Arrange:
		SecretLockRollbackTestContext<test::StateHashDisabledTestContext> context;

		// Act: pair.second contains account state cache state hashes
		auto stateHashesPair = test::Unzip(context.runRegisterLockAndRollbackAndReregisterLockSecretLockTest());

		// Assert:
		test::AssertAllZero(stateHashesPair, 3);
	}

	NO_STRESS_TEST(TEST_CLASS, CanAddAndRollbackAndReaddSecretLockWithStateHashEnabled) {
		// Arrange:
		SecretLockRollbackTestContext<test::StateHashEnabledTestContext> context;

		// Act: pair.second contains account state cache state hashes
		auto stateHashesPair = test::Unzip(context.runRegisterLockAndRollbackAndReregisterLockSecretLockTest());

		// Assert:
		ASSERT_EQ(3u, stateHashesPair.first.size());

		// - state is the same at 0, 1 and changed at 2
		EXPECT_EQ(stateHashesPair.first[0], stateHashesPair.first[1]);
		EXPECT_NE(stateHashesPair.first[1], stateHashesPair.first[2]);

		ASSERT_EQ(3u, stateHashesPair.second.size());

		// - secret lock is active at 0, 1 and expired at 2
		//   when the lock expires, the harvester is credited the lock amount and therefore changing the account state cache
		EXPECT_EQ(stateHashesPair.second[0], stateHashesPair.second[1]);
		EXPECT_NE(stateHashesPair.second[1], stateHashesPair.second[2]);
	}

	// endregion

	// region secret lock (register + unlock) [single chain part]

	namespace {
		template<typename TTestContext>
		SecretLockStateHashes RunLockAndUnlockSecretLockTest(TTestContext& context) {
			// Arrange:
			SecretLockStateHashes stateHashes;
			test::Accounts accounts(4);
			auto stateHashCalculator = context.createStateHashCalculator();

			// - wait for boot
			test::WaitForBoot(context);
			stateHashes.emplace_back(GetStateHash(context), GetComponentStateHash(context));

			// - prepare secret lock
			test::SecretLockTransactionsBuilder transactionsBuilder(accounts);
			transactionsBuilder.addTransfer(0, 2, Amount(1'000'000));
			transactionsBuilder.addTransfer(0, 3, Amount(1'000'000));
			auto secretProof = transactionsBuilder.addSecretLock(2, 3, Amount(100'000), Lock_Duration);

			// - add a second block that unlocks the lock
			transactionsBuilder.addSecretProof(2, 3, secretProof);

			// - send chain
			BlockChainBuilder builder(accounts, stateHashCalculator);
			auto blocks = builder.asBlockChain(transactionsBuilder);

			test::ExternalSourceConnection connection;
			test::PushEntities(connection, ionet::PacketType::Push_Block, blocks);
			test::WaitForHeightAndElements(context, Height(5), 1, 1);
			stateHashes.emplace_back(GetStateHash(context), GetComponentStateHash(context));

			// Assert: the cache has expected balances
			test::AssertCurrencyBalances(accounts, context.localNode().cache(), {
				{ 2, Amount(900'000) },
				{ 3, Amount(1'100'000) }
			});

			return stateHashes;
		}
	}

	NO_STRESS_TEST(TEST_CLASS, CanLockAndUnlockSecretLock_SingleChainPart) {
		// Arrange:
		test::StateHashDisabledTestContext context(test::NonNemesisTransactionPlugins::Lock_Secret, [](auto& config) {
			const_cast<config::NodeConfiguration&>(config.Node).TransactionBatchSize = 50;
		});

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunLockAndUnlockSecretLockTest(context));

		// Assert:
		test::AssertAllZero(stateHashesPair, 2);
	}

	NO_STRESS_TEST(TEST_CLASS, CanLockAndUnlockSecretLockWithStateHashEnabled_SingleChainPart) {
		// Arrange:
		test::StateHashEnabledTestContext context(test::NonNemesisTransactionPlugins::Lock_Secret, [](auto& config) {
			const_cast<config::NodeConfiguration&>(config.Node).TransactionBatchSize = 50;
		});

		// Act + Assert:
		auto stateHashesPair = test::Unzip(RunLockAndUnlockSecretLockTest(context));

		// Assert: all state hashes are nonzero (since importance is recalculated every block none of the hashes are the same)
		test::AssertAllNonZero(stateHashesPair.first, 2);
		test::AssertUnique(stateHashesPair.first);

		// - all secret lock merkle roots are zero because lock is only active inbetween
		test::AssertAllZero(stateHashesPair.second, 2);
	}

	// endregion
}}
