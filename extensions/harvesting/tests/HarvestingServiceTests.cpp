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
		constexpr auto Harvester_Vrf_Key = "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";
		template<uint32_t TAccountVersion>
		HarvestingConfiguration CreateHarvestingConfiguration(bool autoHarvest) {
			auto config = HarvestingConfiguration::Uninitialized();
			config.HarvestKey = Harvester_Key;
			config.HarvesterVrfPrivateKey = Harvester_Vrf_Key;
			auto publicKey = crypto::KeyPair::FromString(config.HarvestKey, TAccountVersion).publicKey();
			config.HarvesterPublicKey = test::ToString(publicKey);
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
		template<uint32_t TAccountVersion>
		class TestContext : public test::ServiceLocatorTestContext<HarvestingServiceTraits> {
		private:
			using BaseType = test::ServiceLocatorTestContext<HarvestingServiceTraits>;

		public:
			explicit TestContext(test::LocalNodeFlags flags = test::LocalNodeFlags::None)
					: BaseType(test::CreateEmptyCatapultCache(test::CreatePrototypicalBlockchainConfiguration())),
					m_config(CreateHarvestingConfiguration<TAccountVersion>(test::LocalNodeFlags::Should_Auto_Harvest == flags)) {

				createConfigurationAccount(TAccountVersion);
				setHooks();
			}

			explicit TestContext(const HarvestingConfiguration& config)
					: BaseType(test::CreateEmptyCatapultCache(test::CreatePrototypicalBlockchainConfiguration()))
					, m_config(config) {
				createConfigurationAccount(TAccountVersion);
				setHooks();
			}
			explicit TestContext(cache::CatapultCache&& cache, const supplier<Timestamp>& timeSupplier = &utils::NetworkTime)
					: BaseType(std::move(cache), timeSupplier)
					, m_config(CreateHarvestingConfiguration<TAccountVersion>(false)) {
				createConfigurationAccount(TAccountVersion);
				setHooks();
			}
			explicit TestContext(cache::CatapultCache&& cache, const HarvestingConfiguration& config, const supplier<Timestamp>& timeSupplier = &utils::NetworkTime)
					: BaseType(std::move(cache), timeSupplier)
					, m_config(config) {
				createConfigurationAccount(TAccountVersion);
				setHooks();
			}


		public:
			Key harvesterKey() const {
				return crypto::KeyPair::FromString(m_config.HarvestKey, TAccountVersion).publicKey();
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
			void setMaxHarvesterBalance(Amount balance) {
				const_cast<model::NetworkConfiguration&>(testState().state().config().Network).MaxHarvesterBalance = balance;
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
			void createConfigurationAccount(uint32_t accountVersion) {
				auto &cache = testState().cache();
				auto cacheDelta = cache.createDelta();
				auto& accountStateCacheDelta = cacheDelta.template sub<cache::AccountStateCache>();
				auto harvestingKeypair = crypto::KeyPair::FromString(m_config.HarvestKey, accountVersion);

				//Ensure account exists so service can boot
				accountStateCacheDelta.addAccount(harvestingKeypair.publicKey(), Height(1), accountVersion);

				cache.commit(Height(1));
			}
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

	namespace {
		using test_types = ::testing::Types<
				std::integral_constant<uint32_t,1>,
				std::integral_constant<uint32_t,2>>;

		template<typename TBaseAccountVersion>
		struct HarvestingServiceTest : public ::testing::Test {};
	}

	TYPED_TEST_CASE(HarvestingServiceTest, test_types);

	// region unlocked accounts

	namespace {
		template<uint32_t TAccountVersion, typename TAction>
		void RunUnlockedAccountsServiceTest(test::LocalNodeFlags localNodeFlags, TAction action) {
			// Arrange:
			TestContext<TAccountVersion> context(localNodeFlags);

			// Act:
			context.boot();

			// Assert:
			EXPECT_EQ(1u, context.locator().numServices());
			EXPECT_EQ(1u, context.locator().counters().size());

			auto pUnlockedAccounts = GetUnlockedAccounts(context.locator());
			ASSERT_TRUE(!!pUnlockedAccounts);
			action(*pUnlockedAccounts, context);
		}
		template<uint32_t TAccountVersion, typename TAction>
		void RunUnlockedAccountsServiceTest(TestContext<TAccountVersion>& context, TAction action) {
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

	TYPED_TEST(HarvestingServiceTest, UnlockedAccountsServiceIsRegisteredProperlyWhenAutoHarvestingIsEnabled) {
		// Arrange:
		RunUnlockedAccountsServiceTest<TypeParam::value>(test::LocalNodeFlags::Should_Auto_Harvest, [](const auto& accounts, const auto& context) {
			// Assert: a single account was unlocked
			EXPECT_TRUE(accounts.view().contains(context.harvesterKey()));
			EXPECT_EQ(1u, context.counter("UNLKED ACCTS"));
		});
	}

		TYPED_TEST(HarvestingServiceTest, UnlockedAccountsServiceIsRegisteredProperlyWhenAutoHarvestingIsDisabled) {
		// Arrange:
		RunUnlockedAccountsServiceTest<TypeParam::value>(test::LocalNodeFlags::None, [](const auto& accounts, const auto& context) {
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
		auto CreateKeyPairs(size_t numKeyPairs, uint32_t accountVersion) {
			std::vector<crypto::KeyPair> keyPairs;
			for (auto i = 0u; i < numKeyPairs; ++i)
				keyPairs.push_back(test::GenerateKeyPair(accountVersion));

			return keyPairs;
		}

		template<uint32_t TAccountVersion>
		void AddAccounts(
				TestContext<TAccountVersion>& context,
				const std::vector<crypto::KeyPair>& keyPairs,
				uint32_t accountVersion,
				const consumer<state::AccountState&>& accountStateModifier = [](const auto&) {}) {
			auto& cache = context.testState().state().cache();
			auto cacheDelta = cache.createDelta();
			auto& accountStateCacheDelta = cacheDelta.template sub<cache::AccountStateCache>();

			for (const auto& keyPair : keyPairs) {
				const auto& publicKey = keyPair.publicKey();
				accountStateCacheDelta.addAccount(publicKey, Height(100), accountVersion);
				accountStateModifier(accountStateCacheDelta.find(publicKey).get());
			}

			cache.commit(Height(100));
		}

		template<uint32_t TAccountVersion>
		std::vector<crypto::KeyPair> AddAccountsWithImportances(TestContext<TAccountVersion>& context, uint32_t accountVersion, const std::vector<Amount>& balances) {
			auto keyPairs = CreateKeyPairs(balances.size(), accountVersion);
			auto iter = balances.cbegin();
			AddAccounts<TAccountVersion>(context, keyPairs, accountVersion, [iter](auto& accountState) mutable {
			  accountState.Balances.credit(Harvesting_Mosaic_Id, Amount(*iter), Height(100));
			  ++iter;
			});

			return keyPairs;
		}
		template<uint32_t TAccountVersion>
		void RunUnlockedAccountsPrioritizationTest(
				DelegatePrioritizationPolicy prioritizationPolicy,
				std::initializer_list<size_t> expectedIndexes) {
			// Arrange:
			auto config = CreateHarvestingConfiguration<TAccountVersion>(true);

			config.MaxUnlockedAccounts = 5;
			config.DelegatePrioritizationPolicy = prioritizationPolicy;
			test::MutableBlockchainConfiguration blockchainConfiguration;

			blockchainConfiguration.Immutable.HarvestingMosaicId = Harvesting_Mosaic_Id;
			blockchainConfiguration.Network.ImportanceGrouping = Importance_Grouping_Delegate_Testing;
			auto cache = test::CreateEmptyCatapultCache(blockchainConfiguration.ToConst(), cache::CacheConfiguration());
			TestContext<TAccountVersion> context(std::move(cache), config);
			//Additional unlocked accounts are always V2 since remote delegated harvesting is not enabled for V1.
			auto keyPairs = AddAccountsWithImportances<TAccountVersion>(context, 2, {
					Amount(100), Amount(200), Amount(50), Amount(150), Amount(250)
			});

			RunUnlockedAccountsServiceTest(context, [&expectedIndexes, &context, &keyPairs](auto& unlockedAccounts) {
			  // Act:
			  std::vector<Key> publicKeys;
			  for (auto& keyPair : keyPairs) {
				  publicKeys.push_back(keyPair.publicKey());
				  unlockedAccounts.modifier().add(BlockGeneratorAccountDescriptor(std::move(keyPair), test::GenerateVrfKeyPair(), 2));
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

	TYPED_TEST(HarvestingServiceTest, UnlockedAccountsServiceIsRegisteredProperlyWhenAutoHarvestingIsEnabledWithAgePrioritizationPolicy) {
		RunUnlockedAccountsPrioritizationTest<TypeParam::value>(DelegatePrioritizationPolicy::Age, { 0, 1, 2, 3 });
	}

	TYPED_TEST(HarvestingServiceTest, UnlockedAccountsServiceIsRegisteredProperlyWhenAutoHarvestingIsEnabledWithImportancePrioritizationPolicy) {
		RunUnlockedAccountsPrioritizationTest<TypeParam::value>(DelegatePrioritizationPolicy::Importance, { 4, 1, 3, 0 });
	}
	namespace {
		void AddHarvestersFileRequestsAndCacheCorrespondingAccounts(cache::CatapultCache& cache, const std::string& filename, const Key& nodeOwnerPublicKey, size_t numRequests) {
			UnlockedAccountsStorage storage(filename);
			auto delta = cache.createDelta();
			for (auto i = 0u; i < numRequests; ++i) {
				auto decryptedBuffer = test::GenerateRandomVector(Key::Size*2);
				auto& accountStateCache = delta.sub<cache::AccountStateCache>();
				auto iter = decryptedBuffer.begin();
				auto publicKey = crypto::KeyPair::FromPrivate(crypto::PrivateKey::Generate([&iter]() mutable { return *iter++; }),2).publicKey();
				accountStateCache.addAccount(publicKey, Height(1));
				auto encryptedPayload = test::PrepareHarvestRequestEncryptedPayload(nodeOwnerPublicKey, decryptedBuffer);
				storage.add(test::GetRequestIdentifier(encryptedPayload), encryptedPayload.Data, Key{ { static_cast<uint8_t>(i) } });
			}
			cache.commit(Height(1));
		}
	}
		TYPED_TEST(HarvestingServiceTest, UnlockedAccountsServiceIsLoadingUnlockedHarvestersFile) {
		// Arrange:
		test::TempDirectoryGuard directoryGuard;
		TestContext<TypeParam::value> context(test::LocalNodeFlags::None);
		context.setDataDirectory(directoryGuard.name());

		auto filename = config::CatapultDataDirectory(directoryGuard.name()).rootDir().file("harvesters.dat");
			AddHarvestersFileRequestsAndCacheCorrespondingAccounts(context.testState().cache(), filename, context.locator().keyPair().publicKey(), 3);

		RunUnlockedAccountsServiceTest<TypeParam::value>(context, [&context](const auto& unlockedAccounts) {
		  // Assert: only accounts from the file were unlocked
		  EXPECT_EQ(3u, context.counter("UNLKED ACCTS"));
		  EXPECT_FALSE(unlockedAccounts.view().contains(context.harvesterKey()));
		});
	}
	// end region unlocked accounts management
	// region harvesting task - utils + basic

	namespace {
		template<uint32_t TAccountVersion, typename TAction>
		void RunTaskTest(TestContext<TAccountVersion>& context, const std::string& taskName, TAction&& action) {
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


		template<uint32_t TAccountVersion>
		auto CreateCacheWithAccount(
				const cache::CacheConfiguration& cacheConfig,
				Height height,
				const Key& publicKey,
				const Key& vrfPublicKey,
				Amount balance) {
			// Arrange:
			test::MutableBlockchainConfiguration config;
			config.Immutable.HarvestingMosaicId = Harvesting_Mosaic_Id;
			config.Network.ImportanceGrouping = Importance_Grouping;
			config.Network.AccountVersion = TAccountVersion;
			auto cache = test::CreateEmptyCatapultCache(config.ToConst(), cacheConfig);
			auto delta = cache.createDelta();

			// - add an account
			auto& accountStateCache = delta.sub<cache::AccountStateCache>();
			accountStateCache.addAccount(publicKey, Height(1));
			auto& accountState = accountStateCache.find(publicKey).get();
			accountState.SupplementalPublicKeys.vrf().unset();
			accountState.SupplementalPublicKeys.vrf().set(vrfPublicKey);
			accountState.Balances.credit(Harvesting_Mosaic_Id, balance, Height(1));

			// - add a block difficulty info
			auto& blockDifficultyCache = delta.sub<cache::BlockDifficultyCache>();
			blockDifficultyCache.insert(state::BlockDifficultyInfo(Height(1), Timestamp(0), Difficulty(1000)));

			// - commit changes
			cache.commit(height);
			return cache;
		}
		template<uint32_t TAccountVersion>
		auto CreateCacheWithAccount(Height height, const Key& publicKey, const Key& vrfPublicKey, Amount balance) {
			return CreateCacheWithAccount<TAccountVersion>(cache::CacheConfiguration(), height, publicKey, vrfPublicKey, balance);
		}
	}

	TYPED_TEST(HarvestingServiceTest, HarvestingTaskIsScheduled) {
		// Assert:
		test::AssertRegisteredTask(TestContext<TypeParam::value>(), 1, Task_Name);
	}

	// endregion

	// region harvesting task - pruning

		TYPED_TEST(HarvestingServiceTest, HarvestingTaskDoesNotPruneEligibleAccount) {
		// Arrange:
		auto height = Height(2 * Importance_Grouping - 1);
		auto keyPair = test::GenerateKeyPair(2);
		auto vrfKeyPair = test::GenerateVrfKeyPair();
		TestContext<TypeParam::value> context(CreateCacheWithAccount<TypeParam::value>(height, keyPair.publicKey(), vrfKeyPair.publicKey(), Account_Balance));
		context.setMinHarvesterBalance(Account_Balance);
		context.setMaxHarvesterBalance(Amount(UINT64_MAX));
		RunTaskTest(context, Task_Name, [keyPair = std::move(keyPair), vrfKeyPair = std::move(vrfKeyPair)](auto& unlockedAccounts, const auto& task) mutable {
			unlockedAccounts.modifier().add(BlockGeneratorAccountDescriptor(std::move(keyPair), std::move(vrfKeyPair), 2));

			// Act:
			auto result = task.Callback().get();

			// Assert:
			EXPECT_EQ(thread::TaskResult::Continue, result);
			EXPECT_EQ(1u, unlockedAccounts.view().size());
		});
	}

		TYPED_TEST(HarvestingServiceTest, HarvestingTaskDoesPruneAccountIneligibleDueToBalance) {
		// Arrange: ineligible because account balance is too low
		auto height = Height(2 * Importance_Grouping - 1);
		auto keyPair = test::GenerateKeyPair(2);
		auto vrfKeyPair = test::GenerateVrfKeyPair();
		TestContext<TypeParam::value> context(CreateCacheWithAccount<TypeParam::value>(height, keyPair.publicKey(), vrfKeyPair.publicKey(), Account_Balance));
		context.setMinHarvesterBalance(Account_Balance + Amount(1));
		context.setMaxHarvesterBalance(Amount(UINT64_MAX));

		RunTaskTest(context, Task_Name, [keyPair = std::move(keyPair), vrfKeyPair = std::move(vrfKeyPair)](auto& unlockedAccounts, const auto& task) mutable {
			unlockedAccounts.modifier().add(BlockGeneratorAccountDescriptor(std::move(keyPair), std::move(vrfKeyPair), 2));

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
		template<uint32_t TAccountVersion>
		void RunHarvestingStateHashTest(bool enableVerifiableState, Hash256& harvestedStateHash) {
			// Arrange: use a huge amount and a max timestamp to force a hit
			test::TempDirectoryGuard dbDirGuard;
			auto keyPair = test::GenerateKeyPair(TAccountVersion);
			auto vrfKeyPair = test::GenerateVrfKeyPair();
			auto balance = Amount(1'000'000'000'000);
			auto cacheConfig = enableVerifiableState
					? cache::CacheConfiguration(dbDirGuard.name(), utils::FileSize(), cache::PatriciaTreeStorageMode::Enabled)
					: cache::CacheConfiguration();
			TestContext<TAccountVersion> context(
					CreateCacheWithAccount<TAccountVersion>(cacheConfig, Height(1), keyPair.publicKey(), vrfKeyPair.publicKey(), balance),
					[]() { return Timestamp(std::numeric_limits<int64_t>::max()); });
			context.setMaxHarvesterBalance(Amount(UINT64_MAX));
			if (enableVerifiableState)
				context.enableVerifiableState();

			RunTaskTest(context, Task_Name, [keyPair = std::move(keyPair), vrfKeyPair = std::move(vrfKeyPair), &context, &harvestedStateHash](
					auto& unlockedAccounts,
					const auto& task) mutable {
				unlockedAccounts.modifier().add(BlockGeneratorAccountDescriptor(std::move(keyPair), std::move(vrfKeyPair), TAccountVersion));

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

	TYPED_TEST(HarvestingServiceTest, HarvestingTaskGeneratesZeroStateHashWhenVerifiableStateIsDisabled) {
		// Act:
		Hash256 harvestedStateHash;
		RunHarvestingStateHashTest<TypeParam::value>(false, harvestedStateHash);

		// Assert:
		EXPECT_EQ(Hash256(), harvestedStateHash);
	}

	TYPED_TEST(HarvestingServiceTest, HarvestingTaskGeneratesNonZeroStateHashWhenVerifiableStateIsEnabled) {
		// Act:
		Hash256 harvestedStateHash;
		RunHarvestingStateHashTest<TypeParam::value>(true, harvestedStateHash);

		// Assert:
		EXPECT_NE(Hash256(), harvestedStateHash);
	}

	// endregion
	// region packet handler

	TYPED_TEST(HarvestingServiceTest, PacketHandlerIsNotRegisteredWhenDiagnosticExtensionIsDisabled) {
		// Arrange:
		TestContext<TypeParam::value> context;

		// Act:
		context.boot();

		// Assert:
		EXPECT_EQ(0u, context.testState().state().packetHandlers().size());
	}

	TYPED_TEST(HarvestingServiceTest, PacketHandlerIsRegisteredWhenDiagnosticExtensionIsEnabled) {
		// Arrange:
		TestContext<TypeParam::value> context;
		context.enableDiagnosticExtension();

		// Act:
		context.boot();

		// Assert:
		const auto& packetHandlers = context.testState().state().packetHandlers();
		EXPECT_EQ(1u, packetHandlers.size());
		EXPECT_TRUE(packetHandlers.canProcess(ionet::PacketType::Unlocked_Accounts));
	}

	namespace {
		template<uint32_t TAccountVersion>
		class DiagnosticEnabledTestContext : public TestContext<TAccountVersion> {
		public:
			DiagnosticEnabledTestContext() {
				TestContext<TAccountVersion>::enableDiagnosticExtension();
			}
		};
	}

	namespace {
		auto GetPublicKeys(const std::vector<crypto::KeyPair>& keyPairs) {
			auto keys = test::Apply(true, keyPairs, [](const auto& keyPair) { return keyPair.publicKey(); });
			return std::set<Key>(keys.cbegin(), keys.cend());
		}

		template<uint32_t TAccountVersion>
		void AssertPacketHandlerReturnsUnlockedAccounts(
				std::vector<crypto::KeyPair>&& keyPairs,
				const consumer<const ionet::ServerPacketHandlerContext&>& assertPacket) {
			// Arrange:
			auto config = CreateHarvestingConfiguration<TAccountVersion>(false);

			TestContext<TAccountVersion> context(config);
			AddAccounts(context, keyPairs, 2);
			context.enableDiagnosticExtension();
			context.boot();

			// - add key pairs to unlocked accounts
			auto pUnlockedAccounts = GetUnlockedAccounts(context.locator());
			ASSERT_TRUE(!!pUnlockedAccounts);

			for (auto& keyPair : keyPairs)
				pUnlockedAccounts->modifier().add(BlockGeneratorAccountDescriptor(std::move(keyPair), test::GenerateVrfKeyPair(), 2));

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

	TYPED_TEST(HarvestingServiceTest, PacketHandlerReturnsEmptyPacketWhenNoUnlockedAccountsArePresent) {
		AssertPacketHandlerReturnsUnlockedAccounts<TypeParam::value>(CreateKeyPairs(0, 2), [](const auto& handlerContext) {
		  // Assert: only header is present
		  auto expectedPacketSize = sizeof(ionet::PacketHeader);
		  test::AssertPacketHeader(handlerContext, expectedPacketSize, ionet::PacketType::Unlocked_Accounts);
		});
	}

	TYPED_TEST(HarvestingServiceTest, PacketHandlerReturnsUnlockedAccounts) {
		// Arrange:
		auto keyPairs = CreateKeyPairs(3, 2);
		auto expectedPublicKeys = GetPublicKeys(keyPairs);
		AssertPacketHandlerReturnsUnlockedAccounts<TypeParam::value>(std::move(keyPairs), [&expectedPublicKeys](const auto& handlerContext) {
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
