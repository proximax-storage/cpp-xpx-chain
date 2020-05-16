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
#include "tests/TestHarness.h"
#include "tests/test/LevyTestUtils.h"

namespace catapult { namespace observers {

#define TEST_CLASS PruneLevyHistoryObserverTests
		
	DEFINE_COMMON_OBSERVER_TESTS(PruneLevyHistory)
	
	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::LevyCacheFactory>;
		
		constexpr MosaicId Currency_Mosaic_Id(1234);
		constexpr uint32_t Max_Rollback_Blocks(50);
		constexpr Height Current_Height(55);
		constexpr Height OldestHeight(5);
		
		constexpr auto Network_Identifier = model::NetworkIdentifier::Mijin_Test;
		using CallbackFn = std::function<void (cache::CatapultCacheDelta&)>;
		
		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			config.Network.MaxRollbackBlocks = Max_Rollback_Blocks;
			return config.ToConst();
		}
		
		void RunTest(bool withLevy, bool withHistory, size_t historyCount, CallbackFn setup, CallbackFn result) {
			ObserverTestContext context(NotifyMode::Commit, Current_Height, CreateConfig());
			auto& levyCache = context.cache().sub<cache::LevyCache>();
			
			auto levyData = test::CreateValidMosaicLevy();
			auto levyEntry = test::CreateLevyEntry(MosaicId(123), levyData, withLevy, withHistory, historyCount, OldestHeight );
			levyCache.insert(levyEntry);
			
			/// mark specific height for removal
			levyCache.markHistoryForRemove(MosaicId(123), OldestHeight);
			
			setup(context.cache());
			
			auto pObserver = CreatePruneLevyHistoryObserver();
			model::BlockNotification<1> notification = test::CreateBlockNotification();
			
			// Act:
			test::ObserveNotification(*pObserver, notification, context);
			
			result(context.cache());
		}
	}

	TEST(TEST_CLASS, PruneLevySpecificHistory) {
		
		RunTest(true, true, 3, [](auto&){}, [](cache::CatapultCacheDelta& cache){
			auto& entry = test::GetLevyEntryFromCache(cache, MosaicId(123));
			EXPECT_EQ(entry.updateHistory().size(), 2);
		});
	}
	
	TEST(TEST_CLASS, PruneLevyDeleleteCache) {
		RunTest(false, true, 1, [](auto&){}, [](cache::CatapultCacheDelta& cache){
			auto& levyCache = cache.sub<cache::LevyCache>();
			auto iter = levyCache.find(MosaicId(123));
			EXPECT_EQ(iter.tryGet(), nullptr);
		});
	}
	
	TEST(TEST_CLASS, PruneLevyUnmarkHistory) {

		RunTest(true, true, 3,[](cache::CatapultCacheDelta& cache){
			/// unmark history
			auto& levyCache = cache.sub<cache::LevyCache>();
			levyCache.unmarkHistoryForRemove(MosaicId(123), OldestHeight);
			
			},[](cache::CatapultCacheDelta& cache){
			auto& entry = test::GetLevyEntryFromCache(cache, MosaicId(123));
			EXPECT_EQ(entry.updateHistory().size(), 3);
		});
	}
	
	TEST(TEST_CLASS, PruneLevyMultipleHistory) {
		
		RunTest(true, true, 3,[](cache::CatapultCacheDelta& cache){
			/// add one more history with similar height as the entry to be pruned
			auto& entry = test::GetLevyEntryFromCache(cache, MosaicId(123));
			entry.updateHistory().push_back(std::make_pair(OldestHeight, test::CreateValidMosaicLevy()));
		},[](cache::CatapultCacheDelta& cache){
			auto& entry = test::GetLevyEntryFromCache(cache, MosaicId(123));
			EXPECT_EQ(entry.updateHistory().size(), 2);
		});
	}
}}
