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
#include "catapult/cache_core/AccountStateCache.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS AccountLinkObserverTests

	using ObserverTestContext = test::ObserverTestContextT<test::CoreSystemCacheFactory>;

	DEFINE_COMMON_OBSERVER_TESTS(AccountLink,)

	namespace {
		struct CommitTraits {
			static constexpr auto Notify_Mode = NotifyMode::Commit;
			static constexpr auto Create_Link = model::AccountLinkAction::Link;
			static constexpr auto Remove_Link = model::AccountLinkAction::Unlink;
		};

		struct RollbackTraits {
			static constexpr auto Notify_Mode = NotifyMode::Rollback;
			// during rollback actions need to be reversed to create or remove link
			static constexpr auto Create_Link = model::AccountLinkAction::Unlink;
			static constexpr auto Remove_Link = model::AccountLinkAction::Link;
		};
	}

#define ACCOUNT_LINK_OBSERVER_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Commit) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<CommitTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Rollback) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<RollbackTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	namespace {
		template<typename TAction>
		void RunTwoAccountTest(cache::AccountStateCacheDelta& accountStateCacheDelta, TAction action) {
			// Arrange:
			auto mainAccountKey = test::GenerateRandomByteArray<Key>();
			auto remoteAccountKey = test::GenerateRandomByteArray<Key>();

			accountStateCacheDelta.addAccount(mainAccountKey, Height(444));
			accountStateCacheDelta.addAccount(remoteAccountKey, Height(444));

			auto mainAccountStateIter = accountStateCacheDelta.find(mainAccountKey);
			auto& mainAccountState = mainAccountStateIter.get();

			auto remoteAccountStateIter = accountStateCacheDelta.find(remoteAccountKey);
			auto& remoteAccountState = remoteAccountStateIter.get();

			// Act + Assert:
			action(mainAccountState, remoteAccountState);
		}
	}

	ACCOUNT_LINK_OBSERVER_TEST(ObserverCreatesLink) {
		// Arrange:
		auto context = ObserverTestContext(TTraits::Notify_Mode, Height(888));
		RunTwoAccountTest(context.cache().sub<cache::AccountStateCache>(), [&context](
				const auto& mainAccountState,
				const auto& remoteAccountState) {
			auto mainAccountKey = mainAccountState.PublicKey;
			auto remoteAccountKey = remoteAccountState.PublicKey;

			auto notification = model::RemoteAccountLinkNotification(mainAccountKey, remoteAccountKey, TTraits::Create_Link);
			auto pObserver = CreateAccountLinkObserver();

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert: link was created
			EXPECT_EQ(state::AccountType::Main, mainAccountState.AccountType);
			EXPECT_EQ(remoteAccountKey, mainAccountState.LinkedAccountKey);

			EXPECT_EQ(state::AccountType::Remote, remoteAccountState.AccountType);
			EXPECT_EQ(mainAccountKey, remoteAccountState.LinkedAccountKey);
		});
	}

	ACCOUNT_LINK_OBSERVER_TEST(ObserverRemovesLink) {
		// Arrange:
		auto context = ObserverTestContext(TTraits::Notify_Mode, Height(888));
		RunTwoAccountTest(context.cache().sub<cache::AccountStateCache>(), [&context](auto& mainAccountState, auto& remoteAccountState) {
			auto mainAccountKey = mainAccountState.PublicKey;
			auto remoteAccountKey = remoteAccountState.PublicKey;

			mainAccountState.LinkedAccountKey = remoteAccountKey;
			mainAccountState.AccountType = state::AccountType::Main;

			remoteAccountState.LinkedAccountKey = mainAccountKey;
			remoteAccountState.AccountType = state::AccountType::Remote;

			auto notification = model::RemoteAccountLinkNotification(mainAccountKey, remoteAccountKey, TTraits::Remove_Link);
			auto pObserver = CreateAccountLinkObserver();

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert: link was removed
			EXPECT_EQ(state::AccountType::Unlinked, mainAccountState.AccountType);
			EXPECT_EQ(Key(), mainAccountState.LinkedAccountKey);

			EXPECT_EQ(state::AccountType::Remote_Unlinked, remoteAccountState.AccountType);
			EXPECT_EQ(Key(), remoteAccountState.LinkedAccountKey);
		});
	}
}}
