/**
*** Copyright (c) 2018-present,
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

		auto CreateNotification(const std::vector<model::CosignatoryModification>& modifications) {
			return Notification(static_cast<uint8_t>(modifications.size()), modifications.data());
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
			auto& multisigCacheDelta = context.cache().sub<cache::ReputationCache>();

			// - create initial multisig entry in cache
			for (const auto& initialValue : initialValues) {
				multisigCacheDelta.insert(state::ReputationEntry(initialValue.AccountKey));
				auto& entry = multisigCacheDelta.find(initialValue.AccountKey).get();
				entry.setPositiveInteractions(Reputation{initialValue.PositiveInteractions});
				entry.setNegativeInteractions(Reputation{initialValue.NegativeInteractions});
			}

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert: check the cache
			for (const auto& expectedValue : expectedValues) {
				EXPECT_EQ(expectedValue.EntryExists, multisigCacheDelta.contains(expectedValue.AccountKey));
				if (expectedValue.EntryExists) {
					const auto& entry = multisigCacheDelta.find(expectedValue.AccountKey).get();
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
		RunTest(NotifyMode::Commit, notification,
			{},
			{ { keys[0], true, 1, 0 }, { keys[1], true, 0, 1 } });
	}

	TEST(TEST_CLASS, ReputationUpdate_Commit) {
		// Arrange:
		auto keys = test::GenerateKeys(2);
		Modifications modifications = { { Add, keys[0] }, { Del, keys[1] } };
		auto notification = CreateNotification(modifications);

		// Assert:
		RunTest(NotifyMode::Commit, notification,
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
			NotifyMode::Rollback, notification,
			{},
			{ { keys[0], false, 0, 0 }, { keys[1], false, 0, 0 } });
	}

	TEST(TEST_CLASS, ReputationUpdate_Rollback) {
		// Arrange:
		auto keys = test::GenerateKeys(2);
		Modifications modifications = { { Add, keys[0] }, { Del, keys[1] } };
		auto notification = CreateNotification(modifications);

		// Assert:
		RunTest(NotifyMode::Rollback, notification,
			{ { keys[0], true, 1, 2 }, { keys[1], true, 3, 4 } },
			{ { keys[0], true, 0, 2 }, { keys[1], true, 3, 3 } });
	}
}}
