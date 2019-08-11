/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/CatapultUpgradeTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS CatapultUpgradeObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(CatapultUpgrade, )

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::CatapultUpgradeCacheFactory>;
		using Notification = model::CatapultUpgradeVersionNotification<1>;

		struct CatapultUpgradeValues {
		public:
			catapult::Height Height;
			BlockDuration UpgradePeriod;
			CatapultVersion Version;
		};

		std::unique_ptr<Notification> CreateNotification(const CatapultUpgradeValues& values) {
			return std::make_unique<Notification>(values.UpgradePeriod, values.Version);
		}

		void RunTest(
				NotifyMode mode,
				const Notification& notification,
				bool entryExists,
				const CatapultUpgradeValues& expectedValues) {
			// Arrange:
			auto config = model::BlockChainConfiguration::Uninitialized();
			ObserverTestContext context(mode, expectedValues.Height, config);
			auto pObserver = CreateCatapultUpgradeObserver();

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert: check the cache
			auto& catapultUpgradeCacheDelta = context.cache().sub<cache::CatapultUpgradeCache>();
			auto height = Height{expectedValues.Height.unwrap() + expectedValues.UpgradePeriod.unwrap()};
			EXPECT_EQ(entryExists, catapultUpgradeCacheDelta.contains(height));
			if (entryExists) {
				const auto& entry = catapultUpgradeCacheDelta.find(height).get();
				EXPECT_EQ(expectedValues.Version, entry.catapultVersion());
			}
		}
	}

	TEST(TEST_CLASS, CatapultUpgrade_Commit) {
		// Arrange:
		auto values = CatapultUpgradeValues{ Height{1}, BlockDuration{2}, CatapultVersion{4} };
		auto notification = CreateNotification(values);

		// Assert:
		RunTest(NotifyMode::Commit, *notification, true, values);
	}

	TEST(TEST_CLASS, CatapultUpgrade_Rollback) {
		// Arrange:
		auto values = CatapultUpgradeValues{ Height{2}, BlockDuration{3}, CatapultVersion{5} };
		auto notification = CreateNotification(values);

		// Assert:
		RunTest(NotifyMode::Rollback, *notification, false, values);
	}
}}
