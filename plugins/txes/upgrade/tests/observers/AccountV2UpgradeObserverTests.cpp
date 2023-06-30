/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "sdk/src/extensions/MemoryStream.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/model/Address.h"
#include "src/observers/Observers.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/BlockchainUpgradeTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS AccountV2UpgradeObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(AccountV2Upgrade, )

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::CoreSystemCacheFactory>;
		using Notification = model::AccountV2UpgradeNotification<1>;

		constexpr auto Default_Network_Id = model::NetworkIdentifier::Mijin_Test;
		constexpr auto Harvesting_Mosaic_Id = MosaicId(9876);

		void RunTest(
				NotifyMode mode) {
			// Arrange:

			ObserverTestContext context(mode, Height(1));
			auto pObserver = CreateAccountV2UpgradeObserver();
			auto& delta = context.observerContext().Cache.sub<cache::AccountStateCache>();
			auto remoteKey = test::GenerateRandomByteArray<Key>();
			crypto::KeyPair signer = test::GenerateKeyPair(1);
			auto finalTarget = test::GenerateKeyPair(2);
			auto notification = std::make_unique<Notification>(signer.publicKey(), finalTarget.publicKey());

			auto originalState = std::make_shared<state::AccountState>(model::PublicKeyToAddress(signer.publicKey(), Default_Network_Id), Height(1), 1);
			originalState->Balances.credit(Harvesting_Mosaic_Id, Amount(1000));
			originalState->SupplementalPublicKeys.linked().set(remoteKey);
			originalState->PublicKey = signer.publicKey();
			originalState->PublicKeyHeight = Height(1);
			if(mode == NotifyMode::Commit)
			{
				// Add signer account
				delta.addAccount(*originalState);
			}
			else
			{
				// Add signer account
				delta.addAccount(signer.publicKey(), Height(1), 1);
				auto& account = delta.find(signer.publicKey()).get();
				account.AccountType = state::AccountType::Locked;

				// Add target account

				delta.addAccount(finalTarget.publicKey(), Height(1), 2);
				auto& targetAccount = delta.find(finalTarget.publicKey()).get();
				targetAccount.Balances.credit(Harvesting_Mosaic_Id, Amount(1000));
				targetAccount.SupplementalPublicKeys.linked().set(remoteKey);
				targetAccount.OldState = originalState;
			}



			test::ObserveNotification(*pObserver, *notification, context);

			// Assert: check the cache
			EXPECT_EQ(mode == NotifyMode::Commit, delta.contains(finalTarget.publicKey()));
			if (mode == NotifyMode::Commit) {
				auto account = delta.find(signer.publicKey()).get();
				auto newAccount = delta.find(finalTarget.publicKey()).get();
				// Verify original account state
				EXPECT_TRUE(account.IsLocked());
				EXPECT_TRUE(!account.Balances.size());

				// Verify new account state
				EXPECT_TRUE(newAccount.OldState);
				EXPECT_EQ(newAccount.Balances.size(), 1);
				EXPECT_EQ(newAccount.Balances.get(Harvesting_Mosaic_Id), Amount(1000));
				EXPECT_EQ(newAccount.SupplementalPublicKeys.linked().get(), remoteKey);
				EXPECT_EQ(account.SupplementalPublicKeys.upgrade().get(), newAccount.PublicKey);

			}
			else
			{
				//VERIFY ORIGINAL ACCOUNT
				auto account = delta.find(signer.publicKey()).get();
				auto newAccount = delta.find(finalTarget.publicKey()).tryGet();

				EXPECT_FALSE(newAccount);
				// Verify original account state
				EXPECT_FALSE(account.IsLocked());
				EXPECT_FALSE(account.OldState);
				EXPECT_EQ(account.Balances.size(), 1);
				EXPECT_EQ(account.Balances.get(Harvesting_Mosaic_Id), Amount(1000));
				EXPECT_EQ(account.SupplementalPublicKeys.linked().get(), remoteKey);
				EXPECT_EQ(account.SupplementalPublicKeys.upgrade().get(), Key());

			}
		}
	}

	TEST(TEST_CLASS, AccountV2Upgrade_Commit) {
		// Assert:
		RunTest(NotifyMode::Commit);
	}

	TEST(TEST_CLASS, AccountV2Upgrade_Rollback) {
		// Assert:
		RunTest(NotifyMode::Rollback);
	}
}}
