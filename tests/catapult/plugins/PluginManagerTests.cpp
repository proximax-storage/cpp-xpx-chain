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

#include "catapult/plugins/PluginManager.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "tests/test/cache/SimpleCache.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/core/mocks/MockTransaction.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

namespace catapult { namespace plugins {

#define TEST_CLASS PluginManagerTests

	// region basic

	TEST(TEST_CLASS, CanCreateDefaultStorageConfiguration) {
		// Act:
		StorageConfiguration config;

		// Assert:
		EXPECT_FALSE(config.PreferCacheDatabase);
		EXPECT_TRUE(config.CacheDatabaseDirectory.empty());
	}

	TEST(TEST_CLASS, CanCreateManager) {
		// Arrange:
		test::MutableBlockchainConfiguration config;
		config.Network.BlockPruneInterval = 15;
		config.Network.Inflation = Amount(234);
		auto pConfigHolder = config::CreateMockConfigurationHolder(config.ToConst());
		auto storageConfig = StorageConfiguration();
		storageConfig.CacheDatabaseDirectory = "abc";

		// Act:
		PluginManager manager(pConfigHolder, storageConfig);

		// Assert: compare BlockPruneInterval, CacheDatabaseDirectory and InflationCalculator as sentinel values
		//         because the manager copies the configs
		EXPECT_EQ(15u, manager.config().BlockPruneInterval);
		EXPECT_EQ("abc", manager.storageConfig().CacheDatabaseDirectory);
		EXPECT_EQ(1u, manager.configHolder()->InflationCalculator().size());
		EXPECT_TRUE(manager.configHolder()->InflationCalculator().contains(Height(1), Amount(234)));
	}

	TEST(TEST_CLASS, CanCreateCacheConfiguration) {
		// Arrange:
		auto pConfigHolder = config::CreateMockConfigurationHolder();

		auto storageConfig = StorageConfiguration();
		storageConfig.PreferCacheDatabase = true;
		storageConfig.CacheDatabaseDirectory = "abc";
		storageConfig.MaxCacheDatabaseWriteBatchSize = utils::FileSize::FromKilobytes(23);

		auto assertCacheConfiguration = [](const auto& cacheConfig, const auto& expectedDirectory) {
			EXPECT_TRUE(cacheConfig.ShouldUseCacheDatabase);
			EXPECT_EQ(expectedDirectory, cacheConfig.CacheDatabaseDirectory);
			EXPECT_EQ(utils::FileSize::FromKilobytes(23), cacheConfig.MaxCacheDatabaseWriteBatchSize);
			EXPECT_FALSE(cacheConfig.ShouldStorePatriciaTrees);
		};

		// Act:
		PluginManager manager(
				pConfigHolder,
				storageConfig);

		// Assert: cache configuration is constructed appropriately
		assertCacheConfiguration(manager.cacheConfig("foo"), "abc/foo");
		assertCacheConfiguration(manager.cacheConfig("bar"), "abc/bar");
	}

	// endregion

	// region tx plugins

	TEST(TEST_CLASS, CanRegisterCustomTransactions) {
		// Arrange:
		auto pConfigHolder = config::CreateMockConfigurationHolder();
		PluginManager manager(pConfigHolder, StorageConfiguration());

		// Act:
		for (auto i : { 7, 9, 4 })
			manager.addTransactionSupport(mocks::CreateMockTransactionPlugin(static_cast<model::EntityType>(i)));

		// Assert:
		EXPECT_EQ(3u, manager.transactionRegistry().size());

		for (auto i : { 7, 9, 4 }) {
			auto entityType = static_cast<model::EntityType>(i);
			EXPECT_TRUE(!!manager.transactionRegistry().findPlugin(entityType)) << "type " << i;
		}
	}

	// endregion

	// region cache plugins

	namespace {
		template<size_t CacheId>
		void AddSubCacheWithId(PluginManager& manager) {
			manager.addCacheSupport<test::SimpleCacheStorageTraits>(std::make_unique<test::SimpleCacheT<CacheId>>());
		}

		template<size_t CacheId>
		class CustomSubCachePluginAdapter
				: public cache::SubCachePluginAdapter<test::SimpleCacheT<CacheId>, test::SimpleCacheStorageTraits> {
		public:
			using cache::SubCachePluginAdapter<test::SimpleCacheT<CacheId>, test::SimpleCacheStorageTraits>::SubCachePluginAdapter;
		};

		template<size_t CacheId>
		void AddSubCachePluginWithId(PluginManager& manager) {
			auto pSubCache = std::make_unique<test::SimpleCacheT<CacheId>>();
			manager.addCacheSupport(std::make_unique<CustomSubCachePluginAdapter<CacheId>>(std::move(pSubCache)));
		}
	}

	TEST(TEST_CLASS, CanRegisterCustomCaches) {
		// Arrange:
		auto pConfigHolder = config::CreateMockConfigurationHolder();
		PluginManager manager(pConfigHolder, StorageConfiguration());

		// Act:
		AddSubCacheWithId<7>(manager);
		AddSubCacheWithId<9>(manager);AddSubCacheWithId<4>(manager);
		auto cache = manager.createCache();

		// Assert:
		EXPECT_EQ(3u, cache.storages().size());
		EXPECT_EQ(0u, cache.sub<test::SimpleCacheT<7>>().createView(Height{0})->size());
		EXPECT_EQ(0u, cache.sub<test::SimpleCacheT<9>>().createView(Height{0})->size());
		EXPECT_EQ(0u, cache.sub<test::SimpleCacheT<4>>().createView(Height{0})->size());
	}

	TEST(TEST_CLASS, CanRegisterCustomCachePlugins) {
		// Arrange:
		auto pConfigHolder = config::CreateMockConfigurationHolder();
		PluginManager manager(pConfigHolder, StorageConfiguration());

		// Act:
		AddSubCachePluginWithId<7>(manager);
		AddSubCachePluginWithId<9>(manager);
		AddSubCachePluginWithId<4>(manager);
		auto cache = manager.createCache();

		// Assert:
		EXPECT_EQ(3u, cache.storages().size());
		EXPECT_EQ(0u, cache.sub<test::SimpleCacheT<7>>().createView(Height{0})->size());
		EXPECT_EQ(0u, cache.sub<test::SimpleCacheT<9>>().createView(Height{0})->size());
		EXPECT_EQ(0u, cache.sub<test::SimpleCacheT<4>>().createView(Height{0})->size());
	}

	// endregion

	// region handlers

	namespace {
		using HandlerHook = consumer<ionet::ServerPacketHandlers&, const cache::CatapultCache&>;

		struct NonDiagnosticHandlerTraits {
			static void AddHandlerHook(PluginManager& manager, const HandlerHook& hook) {
				manager.addHandlerHook(hook);
			}

			static void AddHandlers(PluginManager& manager, ionet::ServerPacketHandlers& handlers, const cache::CatapultCache& cache) {
				manager.addHandlers(handlers, cache);
			}
		};

		template<typename THandlerTraits>
		void AssertCanRegisterCustomHandlers() {
			// Arrange:
			auto pConfigHolder = config::CreateMockConfigurationHolder();
			PluginManager manager(pConfigHolder, StorageConfiguration());

			// Act:
			THandlerTraits::AddHandlerHook(manager, [](auto& handlers, const auto&) {
				handlers.registerHandler(static_cast<ionet::PacketType>(7), [](const auto&, const auto&) {});
				handlers.registerHandler(static_cast<ionet::PacketType>(9), [](const auto&, const auto&) {});
			});
			THandlerTraits::AddHandlerHook(manager, [](auto& handlers, const auto&) {
				handlers.registerHandler(static_cast<ionet::PacketType>(4), [](const auto&, const auto&) {});
			});

			ionet::ServerPacketHandlers handlers;
			THandlerTraits::AddHandlers(manager, handlers, manager.createCache());

			// Assert:
			EXPECT_EQ(3u, handlers.size());
			for (auto type : { 7u, 9u, 4u }) {
				ionet::Packet packet;
				packet.Type = static_cast<ionet::PacketType>(type);
				EXPECT_TRUE(handlers.canProcess(packet)) << "packet type" << type;
			}
		}
	}

	TEST(TEST_CLASS, CanRegisterCustomHandlers) {
		// Assert:
		AssertCanRegisterCustomHandlers<NonDiagnosticHandlerTraits>();
	}

	// endregion

	// region diagnostic handlers

	namespace {
		struct DiagnosticHandlerTraits {
			static void AddHandlerHook(PluginManager& manager, const HandlerHook& hook) {
				manager.addDiagnosticHandlerHook(hook);
			}

			static void AddHandlers(PluginManager& manager, ionet::ServerPacketHandlers& handlers, const cache::CatapultCache& cache) {
				manager.addDiagnosticHandlers(handlers, cache);
			}
		};
	}

	TEST(TEST_CLASS, CanRegisterCustomDiagnosticHandlers) {
		// Assert:
		AssertCanRegisterCustomHandlers<DiagnosticHandlerTraits>();
	}

	// endregion

	// region diagnostic counter handlers

	namespace {
		utils::DiagnosticCounter MakeDiagnosticCounter(uint64_t id) {
			return utils::DiagnosticCounter(utils::DiagnosticCounterId(id), []() { return 0; });
		}
	}

	TEST(TEST_CLASS, CanRegisterCustomDiagnosticCounters) {
		// Arrange:
		auto pConfigHolder = config::CreateMockConfigurationHolder();
		PluginManager manager(pConfigHolder, StorageConfiguration());

		// Act:
		manager.addDiagnosticCounterHook([](auto& counters, const auto&) {
			counters.push_back(MakeDiagnosticCounter(7));
			counters.push_back(MakeDiagnosticCounter(9));
		});
		manager.addDiagnosticCounterHook([](auto& counters, const auto&) {
			counters.push_back(MakeDiagnosticCounter(4));
		});

		std::vector<utils::DiagnosticCounter> counters;
		manager.addDiagnosticCounters(counters, manager.createCache());

		// Assert:
		ASSERT_EQ(3u, counters.size());
		EXPECT_EQ("G", counters[0].id().name());
		EXPECT_EQ("I", counters[1].id().name());
		EXPECT_EQ("D", counters[2].id().name());
	}

	// endregion

	// region validators - helpers

	namespace {
		template<typename TNotification, typename... TArgs>
		class NamedValidatorT : public validators::NotificationValidatorT<TNotification, TArgs...> {
		public:
			explicit NamedValidatorT(const std::string& name) : m_name(name)
			{}

		public:
			const std::string& name() const override {
				return m_name;
			}

			validators::ValidationResult validate(const TNotification&, TArgs&&...) const override {
				return validators::ValidationResult::Failure;
			}

		private:
			std::string m_name;
		};

		std::unique_ptr<const validators::NotificationValidatorT<model::AccountPublicKeyNotification<1>>> CreateNamedStatelessValidator(const std::string& name) {
			return std::make_unique<NamedValidatorT<model::AccountPublicKeyNotification<1>>>(name);
		}

		std::unique_ptr<const validators::NotificationValidatorT<model::AccountPublicKeyNotification<1>, const validators::ValidatorContext&>> CreateNamedStatefulValidator(const std::string& name) {
			return std::make_unique<NamedValidatorT<model::AccountPublicKeyNotification<1>, const validators::ValidatorContext&>>(name);
		}
	}

	// endregion

	// region validators - stateless

	TEST(TEST_CLASS, CanRegisterStatelessValidators) {
		// Arrange:
		auto pConfigHolder = config::CreateMockConfigurationHolder();
		PluginManager manager(pConfigHolder, StorageConfiguration());
		manager.addStatelessValidatorHook([](auto& builder) {
			builder.add(CreateNamedStatelessValidator("alpha"));
			builder.add(CreateNamedStatelessValidator("beta"));
		});
		manager.addStatelessValidatorHook([](auto& builder) {
			builder.add(CreateNamedStatelessValidator("gamma"));
		});

		// Act:
		auto pValidator = manager.createStatelessValidator();

		// Assert:
		auto expectedNames = std::vector<std::string>{ "alpha", "beta", "gamma" };
		EXPECT_EQ(expectedNames, pValidator->names());
	}

	namespace {
		validators::ValidationResult ValidateStateless(const PluginManager& manager, bool suppress) {
			auto pValidator = suppress
					? manager.createStatelessValidator([](auto) { return true; })
					: manager.createStatelessValidator();
			auto notification = model::AccountPublicKeyNotification<1>(test::GenerateRandomByteArray<Key>());
			return pValidator->validate(notification);
		}
	}

	TEST(TEST_CLASS, CanCreateStatelessValidatorWithNoSuppressedFailureFiltering) {
		// Arrange:
		auto pConfigHolder = config::CreateMockConfigurationHolder();
		PluginManager manager(pConfigHolder, StorageConfiguration());
		manager.addStatelessValidatorHook([](auto& builder) {
			builder.add(CreateNamedStatelessValidator("alpha"));
		});

		// Act: no suppression
		auto result = ValidateStateless(manager, false);

		// Assert:
		EXPECT_EQ(validators::ValidationResult::Failure, result);
	}

	TEST(TEST_CLASS, CanCreateStatelessValidatorWithCustomSuppressedFailureFiltering) {
		// Arrange:
		auto pConfigHolder = config::CreateMockConfigurationHolder();
		PluginManager manager(pConfigHolder, StorageConfiguration());
		manager.addStatelessValidatorHook([](auto& builder) {
			builder.add(CreateNamedStatelessValidator("alpha"));
		});

		// Act: suppress everything
		auto result = ValidateStateless(manager, true);

		// Assert:
		EXPECT_EQ(validators::ValidationResult::Success, result);
	}

	// endregion

	// region validators - stateful

	TEST(TEST_CLASS, CanRegisterStatefulValidators) {
		// Arrange:
		auto pConfigHolder = config::CreateMockConfigurationHolder();
		PluginManager manager(pConfigHolder, StorageConfiguration());
		manager.addStatefulValidatorHook([](auto& builder) {
			builder.add(CreateNamedStatefulValidator("alpha"));
			builder.add(CreateNamedStatefulValidator("beta"));
		});
		manager.addStatefulValidatorHook([](auto& builder) {
			builder.add(CreateNamedStatefulValidator("gamma"));
		});

		// Act:
		auto pValidator = manager.createStatefulValidator();

		// Assert:
		auto expectedNames = std::vector<std::string>{ "alpha", "beta", "gamma" };
		EXPECT_EQ(expectedNames, pValidator->names());
	}

	namespace {
		validators::ValidationResult ValidateStateful(const PluginManager& manager, bool suppress) {
			auto pValidator = suppress
					? manager.createStatefulValidator([](auto) { return true; })
					: manager.createStatefulValidator();
			auto notification = model::AccountPublicKeyNotification<1>(test::GenerateRandomByteArray<Key>());
			auto cache = cache::CatapultCache({});
			auto config = config::BlockchainConfiguration::Uninitialized();
			auto context = test::CreateValidatorContext(config, Height(123), cache.createView().toReadOnly());
			return pValidator->validate(notification, context);
		}
	}

	TEST(TEST_CLASS, CanCreateStatefulValidatorWithNoSuppressedFailureFiltering) {
		// Arrange:
		auto pConfigHolder = config::CreateMockConfigurationHolder();
		PluginManager manager(pConfigHolder, StorageConfiguration());
		manager.addStatefulValidatorHook([](auto& builder) {
			builder.add(CreateNamedStatefulValidator("alpha"));
		});

		// Act: no suppression
		auto result = ValidateStateful(manager, false);

		// Assert:
		EXPECT_EQ(validators::ValidationResult::Failure, result);
	}

	TEST(TEST_CLASS, CanCreateStatefulValidatorWithCustomSuppressedFailureFiltering) {
		// Arrange:
		auto pConfigHolder = config::CreateMockConfigurationHolder();
		PluginManager manager(pConfigHolder, StorageConfiguration());
		manager.addStatefulValidatorHook([](auto& builder) {
			builder.add(CreateNamedStatefulValidator("alpha"));
		});

		// Act: suppress everything
		auto result = ValidateStateful(manager, true);

		// Assert:
		EXPECT_EQ(validators::ValidationResult::Success, result);
	}

	// endregion

	// region observers

	namespace {
		class NamedObserver : public observers::NotificationObserverT<model::AccountAddressNotification<1>> {
		public:
			explicit NamedObserver(const std::string& name) : m_name(name)
			{}

		public:
			const std::string& name() const override {
				return m_name;
			}

			void notify(const model::AccountAddressNotification<1>&, observers::ObserverContext&) const override {
				CATAPULT_THROW_RUNTIME_ERROR("not implemented in mock");
			}

		private:
			std::string m_name;
		};

		observers::NotificationObserverPointerT<model::AccountAddressNotification<1>> CreateNamedObserver(const std::string& name) {
			return std::make_unique<NamedObserver>(name);
		}

		template<typename TAction>
		void RunObserverTest(TAction action) {
			// Arrange:
			auto pConfigHolder = config::CreateMockConfigurationHolder();
			PluginManager manager(pConfigHolder, StorageConfiguration());
			manager.addObserverHook([](auto& builder) {
				builder.add(CreateNamedObserver("alpha"));
				builder.add(CreateNamedObserver("beta"));
			});
			manager.addTransientObserverHook([](auto& builder) {
				builder.add(CreateNamedObserver("zeta"));
			});
			manager.addObserverHook([](auto& builder) {
				builder.add(CreateNamedObserver("gamma"));
			});
			manager.addTransientObserverHook([](auto& builder) {
				builder.add(CreateNamedObserver("omega"));
			});

			// Act:
			action(manager);
		}
	}

	TEST(TEST_CLASS, CanRegisterObservers_PermanentOnly) {
		// Arrange:
		RunObserverTest([](const auto& manager) {
			// Act:
			auto pObserver = manager.createPermanentObserver();

			// Assert:
			auto expectedNames = std::vector<std::string>{ "alpha", "beta", "gamma" };
			EXPECT_EQ(expectedNames, pObserver->names());
		});
	}

	TEST(TEST_CLASS, CanRegisterObservers_All) {
		// Arrange:
		RunObserverTest([](const auto& manager) {
			// Act:
			auto pObserver = manager.createObserver();

			// Assert: permanent observers run before transient observers
			auto expectedNames = std::vector<std::string>{ "alpha", "beta", "gamma", "zeta", "omega" };
			EXPECT_EQ(expectedNames, pObserver->names());
		});
	}

	// endregion

	// region resolvers

	namespace {
		struct MosaicResolverTraits {
			static auto CreateUnresolved(uint8_t value) {
				return UnresolvedMosaicId(value);
			}

			static auto CreateResolved(uint8_t value) {
				return MosaicId(value);
			}

			static void AddResolver(PluginManager& manager, uint8_t increment, bool result) {
				manager.addMosaicResolver([increment, result](const auto& cache, const auto& unresolved, auto& resolved) {
					// read from cache to ensure it is correct one
					auto subCacheSize = cache.template sub<test::SimpleCacheT<2>>().size();
					resolved = MosaicId(unresolved.unwrap() + increment + subCacheSize);
					return result;
				});
			}
		};

		struct AddressResolverTraits {
			static auto CreateUnresolved(uint8_t value) {
				return UnresolvedAddress{ { { value } } };
			}

			static auto CreateResolved(uint8_t value) {
				return Address{ { value } };
			}

			static void AddResolver(PluginManager& manager, uint8_t increment, bool result) {
				manager.addAddressResolver([increment, result](const auto& cache, const auto& unresolved, auto& resolved) {
					// read from cache to ensure it is correct one
					auto subCacheSize = cache.template sub<test::SimpleCacheT<2>>().size();
					resolved = Address{ { static_cast<uint8_t>(unresolved[0] + increment + subCacheSize) } };
					return result;
				});
			}
		};
	}

#define RESOLVER_TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Mosaic) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<MosaicResolverTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Address) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<AddressResolverTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	namespace {
		template<typename TUnresolved>
		auto Resolve(PluginManager& manager, const TUnresolved& unresolved) {
			// Arrange: add a sub cache with size one
			AddSubCachePluginWithId<2>(manager);

			auto cache = manager.createCache();
			auto cacheDelta = cache.createDelta();
			cacheDelta.sub<test::SimpleCacheT<2>>().increment();
			auto readOnlyCache = cacheDelta.toReadOnly();

			// Act:
			auto resolverContext = manager.createResolverContext(readOnlyCache);
			return resolverContext.resolve(unresolved);
		}
	}

	RESOLVER_TRAITS_BASED_TEST(CanCreateDefaultResolver) {
		// Arrange:
		auto pConfigHolder = config::CreateMockConfigurationHolder();
		PluginManager manager(pConfigHolder, StorageConfiguration());

		// Act:
		auto result = Resolve(manager, TTraits::CreateUnresolved(123));

		// Assert:
		EXPECT_EQ(TTraits::CreateResolved(123), result);
	}

	RESOLVER_TRAITS_BASED_TEST(CanCreateCustomResolverAroundMatchingResolver) {
		// Arrange:
		auto pConfigHolder = config::CreateMockConfigurationHolder();
		PluginManager manager(pConfigHolder, StorageConfiguration());
		TTraits::AddResolver(manager, 1, true);

		// Act:
		auto result = Resolve(manager, TTraits::CreateUnresolved(123));

		// Assert:
		EXPECT_EQ(TTraits::CreateResolved(123 + 1 + 1), result);
	}

	RESOLVER_TRAITS_BASED_TEST(CanCreateCustomResolverAroundNonMatchingResolver) {
		// Arrange:
		auto pConfigHolder = config::CreateMockConfigurationHolder();
		PluginManager manager(pConfigHolder, StorageConfiguration());
		TTraits::AddResolver(manager, 1, false);

		// Act:
		auto result = Resolve(manager, TTraits::CreateUnresolved(123));

		// Assert:
		EXPECT_EQ(TTraits::CreateResolved(123), result);
	}

	RESOLVER_TRAITS_BASED_TEST(CanCreateCustomResolverThatOnlyExecutesFirstMatchingResolver) {
		// Arrange:
		auto pConfigHolder = config::CreateMockConfigurationHolder();
		PluginManager manager(pConfigHolder, StorageConfiguration());
		TTraits::AddResolver(manager, 1, false);
		TTraits::AddResolver(manager, 2, true); // second one should match
		TTraits::AddResolver(manager, 3, true);

		// Act:
		auto result = Resolve(manager, TTraits::CreateUnresolved(123));

		// Assert:
		EXPECT_EQ(TTraits::CreateResolved(123 + 2 + 1), result);
	}

	// endregion

	// region notification publisher

	namespace {
		constexpr auto Currency_Mosaic_Id = MosaicId(1234);

		template<typename TPublisherFactory>
		void AssertCanCreateNotificationPublisher(size_t expectedNumNotifications, TPublisherFactory publisherFactory) {
			// Arrange:
			test::MutableBlockchainConfiguration config;
			config.Immutable.CurrencyMosaicId = Currency_Mosaic_Id;
			auto pConfigHolder = config::CreateMockConfigurationHolder(config.ToConst());
			PluginManager manager(pConfigHolder, StorageConfiguration());
			manager.addTransactionSupport(mocks::CreateMockTransactionPlugin());

			auto pTransaction = mocks::CreateMockTransaction(0);
			mocks::MockNotificationSubscriber subscriber;
			mocks::MockTypedNotificationSubscriber<model::BalanceDebitNotification<1>> feeSubscriber;

			// Act: create a publisher and publish a transaction
			auto pPublisher = publisherFactory(manager);
			pPublisher->publish(model::WeakEntityInfo(*pTransaction, Height{0}), subscriber);
			pPublisher->publish(model::WeakEntityInfo(*pTransaction, Height{0}), feeSubscriber);

			// Assert: all expected notifications were raised
			EXPECT_EQ(expectedNumNotifications, subscriber.notificationTypes().size());

			// - correct fee mosaic id was passed down
			ASSERT_EQ(1u, feeSubscriber.numMatchingNotifications());
			EXPECT_EQ(extensions::CastToUnresolvedMosaicId(Currency_Mosaic_Id), feeSubscriber.matchingNotifications()[0].MosaicId);
		}
	}

	TEST(TEST_CLASS, CanCreateDefaultNotificationPublisher) {
		// Assert: 8 basic and 1 custom notifications should be raised
		AssertCanCreateNotificationPublisher(8u + 1, [](const auto& manager) {
			return manager.createNotificationPublisher();
		});
	}

	TEST(TEST_CLASS, CanCreateCustomNotificationPublisher) {
		// Assert: 8 basic notifications should be raised
		AssertCanCreateNotificationPublisher(8u, [](const auto& manager) {
			return manager.createNotificationPublisher(model::PublicationMode::Basic);
		});
	}

	// endregion
}}
