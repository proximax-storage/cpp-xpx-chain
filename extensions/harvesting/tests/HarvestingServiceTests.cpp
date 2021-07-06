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

#include <harvesting/src/UnlockedAccountsStorage.h>
#include "harvesting/src/HarvestingService.h"
#include "harvesting/src/HarvestingConfiguration.h"
#include "harvesting/src/UnlockedAccounts.h"
#include "catapult/cache_core/BlockDifficultyCache.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/local/ServiceLocatorTestContext.h"
#include "tests/test/local/ServiceTestUtils.h"
#include "tests/test/core/PacketPayloadTestUtils.h"
#include "tests/test/nodeps/Filesystem.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"
#include "test/HarvestRequestEncryptedPayload.h"
#include "catapult/config/CatapultDataDirectory.h"
#include "tests/test/nodeps/Functional.h"

namespace catapult { namespace harvesting {

#define TEST_CLASS HarvestingServiceTests

	namespace {
		constexpr auto Service_Name = "unlockedAccounts";
		constexpr auto Task_Name = "harvesting task";
		constexpr auto Harvester_Key = "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";

		HarvestingConfiguration CreateHarvestingConfiguration(bool autoHarvest) {
			auto config = HarvestingConfiguration::Uninitialized();
			config.HarvestKey = Harvester_Key;
			config.IsAutoHarvestingEnabled = autoHarvest;
			config.MaxUnlockedAccounts = 10;
			config.Beneficiary = std::string(64, '0');
			return config;
		}

		struct HarvestingServiceTraits {
			static auto CreateRegistrar(const HarvestingConfiguration& config) {
				return CreateHarvestingServiceRegistrar(config);
			}

			static auto CreateRegistrar() {
				return CreateRegistrar(HarvestingConfiguration::Uninitialized());
			}
		};

		class TestContext : public test::ServiceLocatorTestContext<HarvestingServiceTraits> {
		private:
			using BaseType = test::ServiceLocatorTestContext<HarvestingServiceTraits>;

		public:
			explicit TestContext(test::LocalNodeFlags flags = test::LocalNodeFlags::None)
					: m_config(CreateHarvestingConfiguration(test::LocalNodeFlags::Should_Auto_Harvest == flags)) {
				setHooks();
			}

			explicit TestContext(const HarvestingConfiguration& config)
					: BaseType(test::CreateEmptyCatapultCache(test::CreatePrototypicalBlockchainConfiguration()))
					, m_config(config) {
				setHooks();
			}
			explicit TestContext(cache::CatapultCache&& cache, const supplier<Timestamp>& timeSupplier = &utils::NetworkTime)
					: BaseType(std::move(cache), timeSupplier)
					, m_config(CreateHarvestingConfiguration(false)) {
				setHooks();
			}
			explicit TestContext(cache::CatapultCache&& cache, const HarvestingConfiguration& config, const supplier<Timestamp>& timeSupplier = &utils::NetworkTime)
					: BaseType(std::move(cache), timeSupplier)
					, m_config(config) {
				setHooks();
			}


		public:
			Key harvesterKey() const {
				return crypto::KeyPair::FromString(m_config.HarvestKey).publicKey();
			}

			const auto& capturedStateHashes() const {
				return m_capturedStateHashes;
			}

			const auto& capturedSourcePublicKeys() const {
				return m_capturedSourcePublicKeys;
			}

		public:
			void setMinHarvesterBalance(Amount balance) {
				const_cast<model::NetworkConfiguration&>(testState().state().config().Network).MinHarvesterBalance = balance;
			}
			Amount getMinHarvesterBalance() {
				return testState().state().config().Network.MinHarvesterBalance;
			}
			void enableVerifiableState() {
				auto& config = testState().state().config();
				const_cast<bool&>(config.Node.ShouldUseCacheDatabaseStorage) = true;
				const_cast<bool&>(config.Immutable.ShouldEnableVerifiableState) = true;
			}
			void setDataDirectory(const std::string& dataDirectory) {
				auto& config = testState().state().config();
				const_cast<std::string&>(config.User.DataDirectory) = dataDirectory;
			}
			void enableDiagnosticExtension() {
				auto& config = testState().state().config();
				const_cast<std::vector<std::string>&>(config.Extensions.Names).push_back("extension.diagnostics");
			}

		public:
			void boot() {
				ServiceLocatorTestContext::boot(m_config);
			}

		private:
			void setHooks() {
				// set up hooks
				auto& capturedStateHashes = m_capturedStateHashes;
				auto& capturedSourcePublicKeys = m_capturedSourcePublicKeys;
				testState().state().hooks().setCompletionAwareBlockRangeConsumerFactory([&](auto) {
					return [&](auto&& blockRange, auto) {
						for (const auto& block : blockRange.Range)
							capturedStateHashes.push_back(block.StateHash);

						capturedSourcePublicKeys.push_back(blockRange.SourcePublicKey);
						return disruptor::DisruptorElementId();
					};
				});
			}

		private:
			HarvestingConfiguration m_config;
			std::vector<Hash256> m_capturedStateHashes;
			std::vector<Key> m_capturedSourcePublicKeys;
		};

		std::shared_ptr<UnlockedAccounts> GetUnlockedAccounts(const extensions::ServiceLocator& locator) {
			return locator.service<UnlockedAccounts>(Service_Name);
		}
	}

	ADD_SERVICE_REGISTRAR_INFO_TEST(Harvesting, Post_Range_Consumers)

	// region unlocked accounts

	namespace {
		template<typename TAction>
		void RunUnlockedAccountsServiceTest(test::LocalNodeFlags localNodeFlags, TAction action) {
			// Arrange:
			TestContext context(localNodeFlags);

			// Act:
			context.boot();

			// Assert:
			EXPECT_EQ(1u, context.locator().numServices());
			EXPECT_EQ(1u, context.locator().counters().size());

			auto pUnlockedAccounts = GetUnlockedAccounts(context.locator());
			ASSERT_TRUE(!!pUnlockedAccounts);
			action(*pUnlockedAccounts, context);
		}
		template<typename TAction>
		void RunUnlockedAccountsServiceTest(TestContext& context, TAction action) {
			// Act:
			context.boot();

			// Assert:
			EXPECT_EQ(1u, context.locator().numServices());
			EXPECT_EQ(1u, context.locator().counters().size());

			auto pUnlockedAccounts = GetUnlockedAccounts(context.locator());
			ASSERT_TRUE(!!pUnlockedAccounts);
			action(*pUnlockedAccounts);
		}
	}

	TEST(TEST_CLASS, UnlockedAccountsServiceIsRegisteredProperlyWhenAutoHarvestingIsEnabled) {
		// Arrange:
		RunUnlockedAccountsServiceTest(test::LocalNodeFlags::Should_Auto_Harvest, [](const auto& accounts, const auto& context) {
			// Assert: a single account was unlocked
			EXPECT_TRUE(accounts.view().contains(context.harvesterKey()));
			EXPECT_EQ(1u, context.counter("UNLKED ACCTS"));
		});
	}

	TEST(TEST_CLASS, UnlockedAccountsServiceIsRegisteredProperlyWhenAutoHarvestingIsDisabled) {
		// Arrange:
		RunUnlockedAccountsServiceTest(test::LocalNodeFlags::None, [](const auto& accounts, const auto& context) {
			// Assert: no accounts were unlocked
			EXPECT_FALSE(accounts.view().contains(context.harvesterKey()));
			EXPECT_EQ(0u, context.counter("UNLKED ACCTS"));
		});
	}
	// endregion

	// region unlocked accounts management

	namespace {
		constexpr auto Harvesting_Mosaic_Id = MosaicId(9876);
		constexpr auto Importance_Grouping = 234u;
		constexpr auto Importance_Grouping_Delegate_Testing = 0u;
		auto CreateKeyPairs(size_t numKeyPairs) {
			std::vector<crypto::KeyPair> keyPairs;
			for (auto i = 0u; i < numKeyPairs; ++i)
				keyPairs.push_back(test::GenerateKeyPair());

			return keyPairs;
		}

		void AddAccounts(
				TestContext& context,
				const std::vector<crypto::KeyPair>& keyPairs,
				const consumer<state::AccountState&>& accountStateModifier = [](const auto&) {}) {
			auto& cache = context.testState().state().cache();
			auto cacheDelta = cache.createDelta();
			auto& accountStateCacheDelta = cacheDelta.sub<cache::AccountStateCache>();

			for (const auto& keyPair : keyPairs) {
				const auto& publicKey = keyPair.publicKey();
				accountStateCacheDelta.addAccount(publicKey, Height(100));
				accountStateModifier(accountStateCacheDelta.find(publicKey).get());
			}

			cache.commit(Height(100));
		}

		std::vector<crypto::KeyPair> AddAccountsWithImportances(TestContext& context, const std::vector<Amount>& balances) {
			auto keyPairs = CreateKeyPairs(balances.size());
			auto iter = balances.cbegin();
			AddAccounts(context, keyPairs, [iter](auto& accountState) mutable {
			  accountState.Balances.credit(Harvesting_Mosaic_Id, Amount(*iter), Height(100));
			  ++iter;
			});

			return keyPairs;
		}

		void RunUnlockedAccountsPrioritizationTest(
				DelegatePrioritizationPolicy prioritizationPolicy,
				std::initializer_list<size_t> expectedIndexes) {
			// Arrange:
			auto config = CreateHarvestingConfiguration(true);

			config.MaxUnlockedAccounts = 5;
			config.DelegatePrioritizationPolicy = prioritizationPolicy;
			test::MutableBlockchainConfiguration blockchainConfiguration;

			blockchainConfiguration.Immutable.HarvestingMosaicId = Harvesting_Mosaic_Id;
			blockchainConfiguration.Network.ImportanceGrouping = Importance_Grouping_Delegate_Testing;
			auto cache = test::CreateEmptyCatapultCache(blockchainConfiguration.ToConst(), cache::CacheConfiguration());
			TestContext context(std::move(cache), config);
			auto keyPairs = AddAccountsWithImportances(context, {
					Amount(100), Amount(200), Amount(50), Amount(150), Amount(250)
			});

			RunUnlockedAccountsServiceTest(context, [&expectedIndexes, &context, &keyPairs](auto& unlockedAccounts) {
			  // Act:
			  std::vector<Key> publicKeys;
			  for (auto& keyPair : keyPairs) {
				  publicKeys.push_back(keyPair.publicKey());
				  unlockedAccounts.modifier().add(std::move(keyPair));
			  }

			  // Assert:
			  EXPECT_EQ(5u, context.counter("UNLKED ACCTS"));

			  auto unlockedAccountsView = unlockedAccounts.view();
			  EXPECT_TRUE(unlockedAccountsView.contains(context.harvesterKey()));
			  for (auto i : expectedIndexes)
				  EXPECT_TRUE(unlockedAccountsView.contains(publicKeys[i])) << "public key " << i;
			});
		}
	}

	TEST(TEST_CLASS, UnlockedAccountsServiceIsRegisteredProperlyWhenAutoHarvestingIsEnabledWithAgePrioritizationPolicy) {
		RunUnlockedAccountsPrioritizationTest(DelegatePrioritizationPolicy::Age, { 0, 1, 2, 3 });
	}

	TEST(TEST_CLASS, UnlockedAccountsServiceIsRegisteredProperlyWhenAutoHarvestingIsEnabledWithImportancePrioritizationPolicy) {
		RunUnlockedAccountsPrioritizationTest(DelegatePrioritizationPolicy::Importance, { 4, 1, 3, 0 });
	}
	namespace {
		void AddHarvestersFileRequests(const std::string& filename, const Key& nodeOwnerPublicKey, size_t numRequests) {
			UnlockedAccountsStorage storage(filename);
			for (auto i = 0u; i < numRequests; ++i) {
				auto decryptedBuffer = test::GenerateRandomVector(2 * Key::Size);
				auto encryptedPayload = test::PrepareHarvestRequestEncryptedPayload(nodeOwnerPublicKey, decryptedBuffer);
				storage.add(test::GetRequestIdentifier(encryptedPayload), encryptedPayload.Data, Key{ { static_cast<uint8_t>(i) } });
			}
		}
	}
	TEST(TEST_CLASS, UnlockedAccountsServiceIsLoadingUnlockedHarvestersFile) {
		// Arrange:
		test::TempDirectoryGuard directoryGuard;
		TestContext context(test::LocalNodeFlags::None);
		context.setDataDirectory(directoryGuard.name());

		auto filename = config::CatapultDataDirectory(directoryGuard.name()).rootDir().file("harvesters.dat");
		AddHarvestersFileRequests(filename, context.locator().keyPair().publicKey(), 3);

		RunUnlockedAccountsServiceTest(context, [&context](const auto& unlockedAccounts) {
		  // Assert: only accounts from the file were unlocked
		  EXPECT_EQ(3u, context.counter("UNLKED ACCTS"));
		  EXPECT_FALSE(unlockedAccounts.view().contains(context.harvesterKey()));
		});
	}
	// end region unlocked accounts management
	// region harvesting task - utils + basic

	namespace {
		template<typename TAction>
		void RunTaskTest(TestContext& context, const std::string& taskName, TAction&& action) {
			// Act:
			test::RunTaskTest(context, 1, taskName, [&context, action = std::move(action)](const auto& task) mutable {
				auto pUnlockedAccounts = GetUnlockedAccounts(context.locator());
				ASSERT_TRUE(!!pUnlockedAccounts);
				action(*pUnlockedAccounts, task);
			});
		}
	}

	namespace {
		constexpr Amount Account_Balance(1000);



		auto CreateCacheWithAccount(
				const cache::CacheConfiguration& cacheConfig,
				Height height,
				const Key& publicKey,
				Amount balance) {
			// Arrange:
			test::MutableBlockchainConfiguration config;
			config.Immutable.HarvestingMosaicId = Harvesting_Mosaic_Id;
			config.Network.ImportanceGrouping = Importance_Grouping;
			auto cache = test::CreateEmptyCatapultCache(config.ToConst(), cacheConfig);
			auto delta = cache.createDelta();

			// - add an account
			auto& accountStateCache = delta.sub<cache::AccountStateCache>();
			accountStateCache.addAccount(publicKey, Height(1));
			auto& accountState = accountStateCache.find(publicKey).get();
			accountState.Balances.credit(Harvesting_Mosaic_Id, balance, Height(1));

			// - add a block difficulty info
			auto& blockDifficultyCache = delta.sub<cache::BlockDifficultyCache>();
			blockDifficultyCache.insert(state::BlockDifficultyInfo(Height(1), Timestamp(0), Difficulty(1000)));

			// - commit changes
			cache.commit(height);
			return cache;
		}

		auto CreateCacheWithAccount(Height height, const Key& publicKey, Amount balance) {
			return CreateCacheWithAccount(cache::CacheConfiguration(), height, publicKey, balance);
		}
	}

	TEST(TEST_CLASS, HarvestingTaskIsScheduled) {
		// Assert:
		test::AssertRegisteredTask(TestContext(), 1, Task_Name);
	}

	// endregion

	// region harvesting task - pruning

	TEST(TEST_CLASS, HarvestingTaskDoesNotPruneEligibleAccount) {
		// Arrange:
		auto height = Height(2 * Importance_Grouping - 1);
		auto keyPair = test::GenerateKeyPair();
		TestContext context(CreateCacheWithAccount(height, keyPair.publicKey(), Account_Balance));
		context.setMinHarvesterBalance(Account_Balance);

		RunTaskTest(context, Task_Name, [keyPair = std::move(keyPair)](auto& unlockedAccounts, const auto& task) mutable {
			unlockedAccounts.modifier().add(std::move(keyPair));

			// Act:
			auto result = task.Callback().get();

			// Assert:
			EXPECT_EQ(thread::TaskResult::Continue, result);
			EXPECT_EQ(1u, unlockedAccounts.view().size());
		});
	}

	TEST(TEST_CLASS, HarvestingTaskDoesPruneAccountIneligibleDueToBalance) {
		// Arrange: ineligible because account balance is too low
		auto height = Height(2 * Importance_Grouping - 1);
		auto keyPair = test::GenerateKeyPair();
		TestContext context(CreateCacheWithAccount(height, keyPair.publicKey(), Account_Balance));
		context.setMinHarvesterBalance(Account_Balance + Amount(1));

		RunTaskTest(context, Task_Name, [keyPair = std::move(keyPair)](auto& unlockedAccounts, const auto& task) mutable {
			unlockedAccounts.modifier().add(std::move(keyPair));

			// Act:
			auto result = task.Callback().get();

			// Assert:
			EXPECT_EQ(thread::TaskResult::Continue, result);
			EXPECT_EQ(0u, unlockedAccounts.view().size());
		});
	}

	// endregion

	// region harvesting task - state hash

	namespace {
		void RunHarvestingStateHashTest(bool enableVerifiableState, Hash256& harvestedStateHash) {
			// Arrange: use a huge amount and a max timestamp to force a hit
			test::TempDirectoryGuard dbDirGuard;
			auto keyPair = test::GenerateKeyPair();
			auto balance = Amount(1'000'000'000'000);
			auto cacheConfig = enableVerifiableState
					? cache::CacheConfiguration(dbDirGuard.name(), utils::FileSize(), cache::PatriciaTreeStorageMode::Enabled)
					: cache::CacheConfiguration();
			TestContext context(
					CreateCacheWithAccount(cacheConfig, Height(1), keyPair.publicKey(), balance),
					[]() { return Timestamp(std::numeric_limits<int64_t>::max()); });
			if (enableVerifiableState)
				context.enableVerifiableState();

			RunTaskTest(context, Task_Name, [keyPair = std::move(keyPair), &context, &harvestedStateHash](
					auto& unlockedAccounts,
					const auto& task) mutable {
				unlockedAccounts.modifier().add(std::move(keyPair));

				// Act:
				auto result = task.Callback().get();

				// Assert: one block should have been harvested
				ASSERT_EQ(1u, context.capturedStateHashes().size());
				harvestedStateHash = context.capturedStateHashes()[0];

				// - source public key is zero indicating harvester
				ASSERT_EQ(1u, context.capturedSourcePublicKeys().size());
				EXPECT_EQ(Key(), context.capturedSourcePublicKeys()[0]);

				// Sanity:
				EXPECT_EQ(thread::TaskResult::Continue, result);
				EXPECT_EQ(1u, unlockedAccounts.view().size());
			});
		}
	}

	TEST(TEST_CLASS, HarvestingTaskGeneratesZeroStateHashWhenVerifiableStateIsDisabled) {
		// Act:
		Hash256 harvestedStateHash;
		RunHarvestingStateHashTest(false, harvestedStateHash);

		// Assert:
		EXPECT_EQ(Hash256(), harvestedStateHash);
	}

	TEST(TEST_CLASS, HarvestingTaskGeneratesNonZeroStateHashWhenVerifiableStateIsEnabled) {
		// Act:
		Hash256 harvestedStateHash;
		RunHarvestingStateHashTest(true, harvestedStateHash);

		// Assert:
		EXPECT_NE(Hash256(), harvestedStateHash);
	}

	// endregion
	// region packet handler

	TEST(TEST_CLASS, PacketHandlerIsNotRegisteredWhenDiagnosticExtensionIsDisabled) {
		// Arrange:
		TestContext context;

		// Act:
		context.boot();

		// Assert:
		EXPECT_EQ(0u, context.testState().state().packetHandlers().size());
	}

	TEST(TEST_CLASS, PacketHandlerIsRegisteredWhenDiagnosticExtensionIsEnabled) {
		// Arrange:
		TestContext context;
		context.enableDiagnosticExtension();

		// Act:
		context.boot();

		// Assert:
		const auto& packetHandlers = context.testState().state().packetHandlers();
		EXPECT_EQ(1u, packetHandlers.size());
		EXPECT_TRUE(packetHandlers.canProcess(ionet::PacketType::Unlocked_Accounts));
	}

	namespace {
		class DiagnosticEnabledTestContext : public TestContext {
		public:
			DiagnosticEnabledTestContext() {
				enableDiagnosticExtension();
			}
		};
	}

	namespace {
		auto GetPublicKeys(const std::vector<crypto::KeyPair>& keyPairs) {
			auto keys = test::Apply(true, keyPairs, [](const auto& keyPair) { return keyPair.publicKey(); });
			return std::set<Key>(keys.cbegin(), keys.cend());
		}

		void AssertPacketHandlerReturnsUnlockedAccounts(
				std::vector<crypto::KeyPair>&& keyPairs,
				const consumer<const ionet::ServerPacketHandlerContext&>& assertPacket) {
			// Arrange:
			auto config = CreateHarvestingConfiguration(false);

			TestContext context(config);
			AddAccounts(context, keyPairs);
			context.enableDiagnosticExtension();
			context.boot();

			// - add key pairs to unlocked accounts
			auto pUnlockedAccounts = GetUnlockedAccounts(context.locator());
			ASSERT_TRUE(!!pUnlockedAccounts);

			for (auto& keyPair : keyPairs)
				pUnlockedAccounts->modifier().add(std::move(keyPair));

			// Sanity:
			EXPECT_EQ(keyPairs.size(), pUnlockedAccounts->view().size());

			// Act:
			const auto& packetHandlers = context.testState().state().packetHandlers();

			// - process unlocked acconuts request
			auto pPacket = ionet::CreateSharedPacket<ionet::Packet>();
			pPacket->Type = ionet::PacketType::Unlocked_Accounts;
			ionet::ServerPacketHandlerContext handlerContext({}, "");
			EXPECT_TRUE(packetHandlers.process(*pPacket, handlerContext));

			assertPacket(handlerContext);
		}
	}

	TEST(TEST_CLASS, PacketHandlerReturnsEmptyPacketWhenNoUnlockedAccountsArePresent) {
		AssertPacketHandlerReturnsUnlockedAccounts(CreateKeyPairs(0), [](const auto& handlerContext) {
		  // Assert: only header is present
		  auto expectedPacketSize = sizeof(ionet::PacketHeader);
		  test::AssertPacketHeader(handlerContext, expectedPacketSize, ionet::PacketType::Unlocked_Accounts);
		});
	}

	TEST(TEST_CLASS, PacketHandlerReturnsUnlockedAccounts) {
		// Arrange:
		auto keyPairs = CreateKeyPairs(3);
		auto expectedPublicKeys = GetPublicKeys(keyPairs);
		AssertPacketHandlerReturnsUnlockedAccounts(std::move(keyPairs), [&expectedPublicKeys](const auto& handlerContext) {
		  // Assert: header is correct and contains the expected number of keys
		  auto expectedPacketSize = sizeof(ionet::PacketHeader) + 3 * Key::Size;
		  test::AssertPacketHeader(handlerContext, expectedPacketSize, ionet::PacketType::Unlocked_Accounts);

		  const auto* pUnlockedPublicKeys = reinterpret_cast<const Key*>(test::GetSingleBufferData(handlerContext));
		  auto unlockedPublicKeys = std::set<Key>(pUnlockedPublicKeys, pUnlockedPublicKeys + 3);

		  EXPECT_EQ(expectedPublicKeys, unlockedPublicKeys);
		});
	}

	// endregion
}}
