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

#include "src/observers/Observers.h"
#include "tests/test/MultisigCacheTestUtils.h"
#include "tests/test/MultisigTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS ModifyMultisigSettingsObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(ModifyMultisigSettings, )

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::MultisigCacheFactory>;
		using Notification = model::ModifyMultisigSettingsNotification;

		auto CreateNotification(const Key& signer, int8_t minRemovalDelta, int8_t minApprovalDelta) {
			return Notification(signer, minRemovalDelta, minApprovalDelta);
		}

		struct MultisigSettings {
		public:
			uint8_t Removal;
			uint8_t Approval;
		};

		struct TestSettings {
		public:
			uint8_t Expected;
			uint8_t Current;
			int8_t Delta;
		};

		void RunTest(
				ObserverTestContext&& context,
				const Key& signer,
				const MultisigSettings& initialSettings,
				const Notification& notification,
				const MultisigSettings& expectedSettings) {
			// Arrange:
			auto pObserver = CreateModifyMultisigSettingsObserver();

			// - seed the cache
			auto& multisigCacheDelta = context.cache().sub<cache::MultisigCache>();

			// - create multisig entry in cache
			{
				multisigCacheDelta.insert(state::MultisigEntry(signer));
				auto& entry = multisigCacheDelta.find(signer).get();
				entry.setMinRemoval(initialSettings.Removal);
				entry.setMinApproval(initialSettings.Approval);
			}

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert: check the cache
			const auto& entry = multisigCacheDelta.find(signer).get();
			EXPECT_EQ(expectedSettings.Removal, entry.minRemoval()) << "initial: " << static_cast<int>(initialSettings.Removal);
			EXPECT_EQ(expectedSettings.Approval, entry.minApproval()) << "initial: " << static_cast<int>(initialSettings.Approval);
		}

		struct CommitTraits {
		public:
			static constexpr NotifyMode Mode = NotifyMode::Commit;

			static void AssertTestWithSettings(const TestSettings& removal, const TestSettings& approval) {
				// Arrange:
				auto signer = test::GenerateRandomData<Key_Size>();
				auto notification = CreateNotification(signer, removal.Delta, approval.Delta);

				// Act + Assert:
				RunTest(
						ObserverTestContext(Mode, Height(777)),
						signer,
						{ removal.Current, approval.Current },
						notification,
						{ removal.Expected, approval.Expected });
			}
		};

		struct RollbackTraits {
		public:
			static constexpr NotifyMode Mode = NotifyMode::Rollback;

			static void AssertTestWithSettings(const TestSettings& removal, const TestSettings& approval) {
				// Arrange:
				auto signer = test::GenerateRandomData<Key_Size>();
				auto notification = CreateNotification(signer, removal.Delta, approval.Delta);

				// Act + Assert:
				RunTest(
						ObserverTestContext(Mode, Height(777)),
						signer,
						{ removal.Expected, approval.Expected },
						notification,
						{ removal.Current, approval.Current });
			}
		};
	}

#define NOTIFY_MODE_BASED_TRAITS(TEST_NAME) \
	template<typename TTraits> void TEST_NAME(); \
	TEST(TEST_CLASS, TEST_NAME##_Commit) { TEST_NAME<CommitTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Rollback) { TEST_NAME<RollbackTraits>(); } \
	template<typename TTraits> void TEST_NAME()

	NOTIFY_MODE_BASED_TRAITS(ObserverIgnoresNotificationIfAccountIsUnknown) {
		// Arrange:
		auto signer = test::GenerateRandomData<Key_Size>();
		auto notification = CreateNotification(signer, 0, 0);

		auto pObserver = CreateModifyMultisigSettingsObserver();
		ObserverTestContext context(TTraits::Mode, Height(777));

		// Act: observer does not throw
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: cache was not altered
		const auto& multisigCache = context.cache().sub<cache::MultisigCache>();
		EXPECT_EQ(0u, multisigCache.size());
	}

	NOTIFY_MODE_BASED_TRAITS(ZeroDeltaDoesNotChangeSettings) {
		// Assert:
		TTraits::AssertTestWithSettings({ 10, 10, 0 }, { 123, 123, 0 });
	}

	NOTIFY_MODE_BASED_TRAITS(PositiveDeltaIncreasesSettings) {
		// Assert:
		TTraits::AssertTestWithSettings({ 22, 10, 12, }, { 157, 123, 34 });
	}

	NOTIFY_MODE_BASED_TRAITS(NegativeDeltaDecreasesSettings) {
		// Assert:
		TTraits::AssertTestWithSettings({ 7, 10, -3 }, { 89, 123, -34 });
	}

	NOTIFY_MODE_BASED_TRAITS(ObserverDoesNotCareAboutWrapAround) {
		// Assert: note: that's something that settings validator checks
		TTraits::AssertTestWithSettings({ 124, 254, 126 }, { 128, 0, -128 });
	}
}}
