/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/ReputationCacheTestUtils.h"
#include "tests/test/ReputationTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS ReputationUpdateObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(ReputationUpdate, )

	namespace {
		constexpr auto Add = model::CosignatoryModificationType::Add;
		constexpr auto Del = model::CosignatoryModificationType::Del;
		using ObserverTestContext = test::ObserverTestContextT<test::ReputationCacheFactory>;
		using Notification = model::ReputationUpdateNotification;
		using Modifications = std::vector<model::CosignatoryModification>;

		std::unique_ptr<model::ReputationUpdateNotification> CreateNotification(const std::vector<model::CosignatoryModification>& modifications) {
			std::vector<const model::CosignatoryModification*> modificationPtrs;

			for (auto i = 0u; i < modifications.size(); ++i) {
				modificationPtrs.emplace_back(&modifications[i]);
			}

			return Notification::CreateReputationUpdateNotification(modificationPtrs);
		}

		struct ReputationValues {
		public:
			Key AccountKey;
			bool EntryExists;
			uint64_t PositiveInteractions;
			uint64_t NegativeInteractions;
		};

		void RunTest(
				NotifyMode mode,
				const Notification& notification,
				const std::vector<ReputationValues> initialValues,
				const std::vector<ReputationValues>& expectedValues) {
			// Arrange:
			ObserverTestContext context{mode, Height(777)};
			auto pObserver = CreateReputationUpdateObserver();

			// - seed the cache
			auto& reputationCacheDelta = context.cache().sub<cache::ReputationCache>();

			// - create initial reputation entry in cache
			for (const auto& initialValue : initialValues) {
				reputationCacheDelta.insert(state::ReputationEntry(initialValue.AccountKey));
				auto& entry = reputationCacheDelta.find(initialValue.AccountKey).get();
				entry.setPositiveInteractions(Reputation{initialValue.PositiveInteractions});
				entry.setNegativeInteractions(Reputation{initialValue.NegativeInteractions});
			}

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert: check the cache
			for (const auto& expectedValue : expectedValues) {
				EXPECT_EQ(expectedValue.EntryExists, reputationCacheDelta.contains(expectedValue.AccountKey));
				if (expectedValue.EntryExists) {
					const auto& entry = reputationCacheDelta.find(expectedValue.AccountKey).get();
					EXPECT_EQ(expectedValue.PositiveInteractions, entry.positiveInteractions().unwrap());
					EXPECT_EQ(expectedValue.NegativeInteractions, entry.negativeInteractions().unwrap());
				}
			}
		}
	}

	TEST(TEST_CLASS, ReputationUpdate_Commit_CacheEmpty) {
		// Arrange:
		auto keys = test::GenerateKeys(2);
		Modifications modifications = { { Add, keys[0] }, { Del, keys[1] } };
		auto notification = CreateNotification(modifications);

		// Assert:
		RunTest(NotifyMode::Commit, *notification,
			{},
			{ { keys[0], true, 1, 0 }, { keys[1], true, 0, 1 } });
	}

	TEST(TEST_CLASS, ReputationUpdate_Commit) {
		// Arrange:
		auto keys = test::GenerateKeys(2);
		Modifications modifications = { { Add, keys[0] }, { Del, keys[1] } };
		auto notification = CreateNotification(modifications);

		// Assert:
		RunTest(NotifyMode::Commit, *notification,
			{ { keys[0], true, 1, 2 }, { keys[1], true, 3, 4 } },
			{ { keys[0], true, 2, 2 }, { keys[1], true, 3, 5 } });
	}

	TEST(TEST_CLASS, ReputationUpdate_Rollback_CacheEmpty) {
		// Arrange:
		auto keys = test::GenerateKeys(2);
		Modifications modifications = { { Add, keys[0] }, { Del, keys[1] } };
		auto notification = CreateNotification(modifications);

		// Assert:
		RunTest(
			NotifyMode::Rollback, *notification,
			{},
			{ { keys[0], false, 0, 0 }, { keys[1], false, 0, 0 } });
	}

	TEST(TEST_CLASS, ReputationUpdate_Rollback) {
		// Arrange:
		auto keys = test::GenerateKeys(2);
		Modifications modifications = { { Add, keys[0] }, { Del, keys[1] } };
		auto notification = CreateNotification(modifications);

		// Assert:
		RunTest(NotifyMode::Rollback, *notification,
			{ { keys[0], true, 1, 2 }, { keys[1], true, 3, 4 } },
			{ { keys[0], true, 0, 2 }, { keys[1], true, 3, 3 } });
	}
}}
