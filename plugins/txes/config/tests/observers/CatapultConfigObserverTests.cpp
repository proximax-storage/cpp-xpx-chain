/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/CatapultConfigTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS CatapultConfigObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(CatapultConfig, )

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::CatapultConfigCacheFactory>;
		using Notification = model::CatapultConfigNotification<1>;

		struct CatapultConfigValues {
		public:
			catapult::Height Height;
			BlockDuration ApplyHeightDelta;
			std::string BlockChainConfig;
			std::string SupportedEntityVersions;
		};

		std::unique_ptr<Notification> CreateNotification(const CatapultConfigValues& values) {
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
				const CatapultConfigValues& expectedValues) {
			// Arrange:
			auto config = model::BlockChainConfiguration::Uninitialized();
			ObserverTestContext context(mode, expectedValues.Height, config);
			auto pObserver = CreateCatapultConfigObserver();

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert: check the cache
			auto& catapultConfigCacheDelta = context.cache().sub<cache::CatapultConfigCache>();
			auto height = Height{expectedValues.Height.unwrap() + expectedValues.ApplyHeightDelta.unwrap()};
			EXPECT_EQ(entryExists, catapultConfigCacheDelta.contains(height));
			if (entryExists) {
				const auto& entry = catapultConfigCacheDelta.find(height).get();
				EXPECT_EQ(expectedValues.BlockChainConfig, entry.blockChainConfig());
				EXPECT_EQ(expectedValues.SupportedEntityVersions, entry.supportedEntityVersions());
			}
		}
	}

	TEST(TEST_CLASS, CatapultConfig_Commit) {
		// Arrange:
		auto values = CatapultConfigValues{ Height{1}, BlockDuration{2}, "aaa", "bbb" };
		auto notification = CreateNotification(values);

		// Assert:
		RunTest(NotifyMode::Commit, *notification, true, values);
	}

	TEST(TEST_CLASS, CatapultConfig_Rollback) {
		// Arrange:
		auto values = CatapultConfigValues{ Height{2}, BlockDuration{3}, "ccc", "ddd" };
		auto notification = CreateNotification(values);

		// Assert:
		RunTest(NotifyMode::Rollback, *notification, false, values);
	}
}}
