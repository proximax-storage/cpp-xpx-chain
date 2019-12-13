/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/core/NotificationTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS DriveCacheBlockPruningObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(DriveCacheBlockPruning, config::CreateMockConfigurationHolder())

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::DriveCacheFactory>;

		constexpr uint32_t Max_Rollback_Blocks(50);
		constexpr Height Prune_Height(55);

		auto CreateConfigHolder() {
			test::MutableBlockchainConfiguration config;
			config.Network.MaxRollbackBlocks = Max_Rollback_Blocks;
			return config::CreateMockConfigurationHolder(config.ToConst());
		}

		state::DriveEntry CreateDriveEntry(const Height& end) {
			state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
			entry.setEnd(end);
			return entry;
		}

		struct CacheValues {
		public:
			explicit CacheValues(
					std::vector<state::DriveEntry>&& initialEntries,
					std::vector<state::DriveEntry>&& expectedEntries)
				: InitialEntries(std::move(initialEntries))
				, ExpectedEntries(std::move(expectedEntries))
			{}

		public:
			std::vector<state::DriveEntry> InitialEntries;
			std::vector<state::DriveEntry> ExpectedEntries;
		};

		void PrepareEntries(
				NotifyMode mode,
				std::vector<state::DriveEntry>& initialEntries,
				std::vector<state::DriveEntry>& expectedEntries) {
			auto initialEntry = CreateDriveEntry(Prune_Height - Height(1));
			initialEntries.push_back(initialEntry);
			expectedEntries.push_back(initialEntry);

			initialEntry = CreateDriveEntry(Prune_Height + Height(1));
			initialEntries.push_back(initialEntry);
			expectedEntries.push_back(initialEntry);

			initialEntry = CreateDriveEntry(Prune_Height);
			initialEntries.push_back(initialEntry);
			if (NotifyMode::Rollback == mode)
				expectedEntries.push_back(initialEntry);
		}

		void RunTest(NotifyMode mode, const CacheValues& values) {
			// Arrange:
			ObserverTestContext context(mode, Prune_Height + Height(Max_Rollback_Blocks));
			auto notification = test::CreateBlockNotification();
			auto pObserver = CreateDriveCacheBlockPruningObserver(CreateConfigHolder());
			auto& driveCache = context.cache().sub<cache::DriveCache>();

			// Populate drive cache.
			for (const auto& initialEntry : values.InitialEntries) {
				driveCache.insert(initialEntry);
				driveCache.markRemoveDrive(initialEntry.key(), initialEntry.end());
			}

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert: check the cache
			for (const auto& expectedEntry : values.ExpectedEntries) {
				auto iter = driveCache.find(expectedEntry.key());
				const auto& actualEntry = iter.get();
				test::AssertEqualDriveData(expectedEntry, actualEntry);
			}

			for (auto i = values.ExpectedEntries.size(); i < values.InitialEntries.size(); ++i) {
				auto iter = driveCache.find(values.InitialEntries[i].key());
				EXPECT_EQ(nullptr, iter.tryGet());
			}
		}
	}

	TEST(TEST_CLASS, DriveCacheBlockPruning_Commit) {
		// Arrange:
		std::vector<state::DriveEntry> initialEntries;
		std::vector<state::DriveEntry> expectedEntries;
		PrepareEntries(NotifyMode::Commit, initialEntries, expectedEntries);
		CacheValues values(std::move(initialEntries), std::move(expectedEntries));

		// Assert:
		RunTest(NotifyMode::Commit, values);
	}

	TEST(TEST_CLASS, DriveCacheBlockPruning_Rollback) {
		// Arrange:
		std::vector<state::DriveEntry> initialEntries;
		std::vector<state::DriveEntry> expectedEntries;
		PrepareEntries(NotifyMode::Rollback, expectedEntries, initialEntries);
		CacheValues values(std::move(initialEntries), std::move(expectedEntries));

		// Assert:
		RunTest(NotifyMode::Rollback, values);
	}
}}
