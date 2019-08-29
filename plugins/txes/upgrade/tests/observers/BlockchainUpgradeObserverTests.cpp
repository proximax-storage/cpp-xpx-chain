/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/BlockchainUpgradeTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS BlockchainUpgradeObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(BlockchainUpgrade, )

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::BlockchainUpgradeCacheFactory>;
		using Notification = model::BlockchainUpgradeVersionNotification<1>;

		struct BlockchainUpgradeValues {
		public:
			catapult::Height Height;
			BlockDuration UpgradePeriod;
			BlockchainVersion Version;
		};

		std::unique_ptr<Notification> CreateNotification(const BlockchainUpgradeValues& values) {
			return std::make_unique<Notification>(values.UpgradePeriod, values.Version);
		}

		void RunTest(
				NotifyMode mode,
				const Notification& notification,
				bool entryExists,
				const BlockchainUpgradeValues& expectedValues) {
			// Arrange:
			auto config = model::NetworkConfiguration::Uninitialized();
			ObserverTestContext context(mode, expectedValues.Height, config);
			auto pObserver = CreateBlockchainUpgradeObserver();

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert: check the cache
			auto& blockchainUpgradeCacheDelta = context.cache().sub<cache::BlockchainUpgradeCache>();
			auto height = Height{expectedValues.Height.unwrap() + expectedValues.UpgradePeriod.unwrap()};
			EXPECT_EQ(entryExists, blockchainUpgradeCacheDelta.contains(height));
			if (entryExists) {
				const auto& entry = blockchainUpgradeCacheDelta.find(height).get();
				EXPECT_EQ(expectedValues.Version, entry.blockChainVersion());
			}
		}
	}

	TEST(TEST_CLASS, BlockchainUpgrade_Commit) {
		// Arrange:
		auto values = BlockchainUpgradeValues{ Height{1}, BlockDuration{2}, BlockchainVersion{4} };
		auto notification = CreateNotification(values);

		// Assert:
		RunTest(NotifyMode::Commit, *notification, true, values);
	}

	TEST(TEST_CLASS, BlockchainUpgrade_Rollback) {
		// Arrange:
		auto values = BlockchainUpgradeValues{ Height{2}, BlockDuration{3}, BlockchainVersion{5} };
		auto notification = CreateNotification(values);

		// Assert:
		RunTest(NotifyMode::Rollback, *notification, false, values);
	}
}}
