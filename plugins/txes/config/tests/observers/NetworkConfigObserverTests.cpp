/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/NetworkConfigTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS NetworkConfigObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(NetworkConfig, )

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::NetworkConfigCacheFactory>;
		using Notification = model::NetworkConfigNotification<1>;

		struct NetworkConfigValues {
		public:
			catapult::Height Height;
			BlockDuration ApplyHeightDelta;
			std::string BlockChainConfig;
			std::string SupportedEntityVersions;
		};

		std::unique_ptr<Notification> CreateNotification(const NetworkConfigValues& values) {
			return std::make_unique<Notification>(
				values.ApplyHeightDelta,
				values.BlockChainConfig.size(),
				reinterpret_cast<const uint8_t*>(values.BlockChainConfig.data()),
				values.SupportedEntityVersions.size(),
				reinterpret_cast<const uint8_t*>(values.SupportedEntityVersions.data()));
		}

		void RunTest(
				NotifyMode mode,
				const Notification& notification,
				bool entryExists,
				const NetworkConfigValues& expectedValues) {
			// Arrange:
			auto config = model::NetworkConfiguration::Uninitialized();
			ObserverTestContext context(mode, expectedValues.Height, config);
			auto pObserver = CreateNetworkConfigObserver();

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert: check the cache
			auto& networkConfigCacheDelta = context.cache().sub<cache::NetworkConfigCache>();
			auto height = Height{expectedValues.Height.unwrap() + expectedValues.ApplyHeightDelta.unwrap()};
			EXPECT_EQ(entryExists, networkConfigCacheDelta.contains(height));
			if (entryExists) {
				const auto& entry = networkConfigCacheDelta.find(height).get();
				EXPECT_EQ(expectedValues.BlockChainConfig, entry.networkConfig());
				EXPECT_EQ(expectedValues.SupportedEntityVersions, entry.supportedEntityVersions());
			}
		}
	}

	TEST(TEST_CLASS, NetworkConfig_Commit) {
		// Arrange:
		auto values = NetworkConfigValues{ Height{1}, BlockDuration{2}, "aaa", "bbb" };
		auto notification = CreateNotification(values);

		// Assert:
		RunTest(NotifyMode::Commit, *notification, true, values);
	}

	TEST(TEST_CLASS, NetworkConfig_Rollback) {
		// Arrange:
		auto values = NetworkConfigValues{ Height{2}, BlockDuration{3}, "ccc", "ddd" };
		auto notification = CreateNotification(values);

		// Assert:
		RunTest(NotifyMode::Rollback, *notification, false, values);
	}
}}
