/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#include "tests/test/cache/BalanceTransferTestUtils.h"
#include "tests/test/LevyTestUtils.h"
#include "src/observers/Observers.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"
#include "plugins/txes/mosaic/src/model/MosaicLevy.h"
#include "src/model/MosaicEntityType.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/test/LevyTestUtils.h"
#include "src/cache/LevyCache.h"

namespace catapult {
	namespace observers {
	
#define TEST_CLASS MosaicLevyBalanceTransferObserverTest
		
		DEFINE_COMMON_OBSERVER_TESTS(LevyBalanceTransfer, )
		
		namespace {
			
			auto Currency_Mosaic_Id = MosaicId(1234);
			auto Levy_Mosaic_Id = MosaicId(333);
			using ObserverTestContext = test::ObserverTestContextT<test::LevyCacheFactory>;
			using LevySetup = std::function<void(cache::CatapultCacheDelta& cache)>;
			
			void RunTest(ObserverTestContext& context, const Key& signer ) {
				// Arrange:
				auto pObserver = CreateLevyBalanceTransferObserver();
				
				// Act
				auto notification = model::BalanceTransferNotification<1>(signer,
					UnresolvedAddress(),test::UnresolveXor(Currency_Mosaic_Id), Amount(123));
				
				// Trigger
				test::ObserveNotification(*pObserver, notification, context);
			}
			
			void AssertTransferResult(observers::NotifyMode mode, bool config, LevySetup setup,
				Address recipient, Amount initialBalances[2], Amount finalBalances[2]) {
				ObserverTestContext context(mode, Height(10), test::CreateMosaicConfigWithLevy(config));
				
				setup(context.cache());
				
				// create sender account with balance
				auto sender = test::GenerateRandomByteArray<Key>();
				test::SetCacheBalances(context.cache(), sender, test::CreateMosaicBalance( Levy_Mosaic_Id, initialBalances[0]));
				test::SetCacheBalances(context.cache(), recipient, test::CreateMosaicBalance( Levy_Mosaic_Id, initialBalances[1]));
				
				RunTest(context, sender);
				
				test::AssertBalances(context.cache(), sender, test::CreateMosaicBalance(Levy_Mosaic_Id, finalBalances[0]));
				test::AssertBalances(context.cache(), recipient, test::CreateMosaicBalance(Levy_Mosaic_Id, finalBalances[1]));
			}
		}
		
		TEST(TEST_CLASS, LevyTransferBalanceCommit) {
			auto levy = test::CreateLevyEntry(Levy_Mosaic_Id, Amount(1000));
			Amount initialBalance[] = { Amount(2000), Amount(10) };
			Amount finalBalance[] = { Amount(2000) - levy.Fee, levy.Fee + Amount(10)};
			
			AssertTransferResult(NotifyMode::Commit, true,
				[levy](auto& cache){
					test::AddMosaicWithLevy(cache, Currency_Mosaic_Id, Height(1), levy, Key());
				},
				levy.Recipient, initialBalance, finalBalance);
		}
		
		TEST(TEST_CLASS, LevyTransferBalanceRollback) {
			auto levy = test::CreateLevyEntry(Levy_Mosaic_Id, Amount(1000));
			Amount initialBalance[] = { Amount(2000), Amount(5000) };
			Amount finalBalance[] = { Amount(2000) + levy.Fee, Amount(5000) - levy.Fee };
			
			AssertTransferResult(NotifyMode::Rollback, true,
				[levy](auto& cache){
					test::AddMosaicWithLevy(cache, Currency_Mosaic_Id, Height(1), levy, Key());
				},
				levy.Recipient, initialBalance, finalBalance);
		}
		
		TEST(TEST_CLASS, LevyTransferBalanceConfigDisabled) {
			Amount initialBalance[] = { Amount(2000), Amount(5000) };
			auto recipient = test::GenerateRandomByteArray<Address>();
			AssertTransferResult(NotifyMode::Commit, false, [](auto&){}, recipient, initialBalance, initialBalance);
		}
		
		TEST(TEST_CLASS, LevyTransferBalanceNoLevy) {
			Amount initialBalance[] = { Amount(2000), Amount(5000) };
			auto recipient = test::GenerateRandomByteArray<Address>();
			AssertTransferResult(NotifyMode::Commit, true, [](auto&){}, recipient, initialBalance, initialBalance);
		}
		
		TEST(TEST_CLASS, LevyTransferBalanceDeletedLevy) {
			Amount initialBalance[] = { Amount(2000), Amount(5000) };
			auto recipient = test::GenerateRandomByteArray<Address>();
			
			AssertTransferResult(NotifyMode::Commit, true,
				[](auto& cache){
					auto& levyCacheDelta = cache.template sub<cache::LevyCache>();
					auto levyEntry = test::CreateLevyEntry(false, false);
					levyCacheDelta.insert(levyEntry);
			     },
			     recipient, initialBalance, initialBalance);
		}
	}
}