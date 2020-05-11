/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#include "tests/test/MosaicCacheTestUtils.h"
#include "src/observers/Observers.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"
#include "plugins/txes/mosaic/src/model/MosaicLevy.h"
#include "catapult/types.h"
#include "src/model/MosaicEntityType.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/test/LevyTestUtils.h"
#include "src/model/MosaicModifyLevyTransaction.h"
#include "src/cache/LevyCache.h"

namespace catapult {
	namespace observers {

#define TEST_CLASS MosaicModifyLevyObserver
	
	/// region Add levy
	DEFINE_COMMON_OBSERVER_TESTS(ModifyLevy, )
	
	namespace {
		
		using ObserverTestContext = test::ObserverTestContextT<test::LevyCacheFactory>;
		
		auto Unresolved_Mosaic_Id = UnresolvedMosaicId(1234);
		auto Currency_Mosaic_Id = MosaicId(Unresolved_Mosaic_Id.unwrap());
		Height height(444);
		
		using LevySetupFunc = std::function<void (cache::CatapultCacheDelta&, const state::LevyEntryData&, const Key&)>;
		using TestResultFunc = std::function<void (cache::CatapultCacheDelta&, const model::MosaicLevyRaw&, const state::LevyEntryData&)>;
		
		void RunModifyTest(ObserverTestContext& context, const model::MosaicLevyRaw& levy) {
			auto pObserver = CreateModifyLevyObserver();
			
			auto owner = test::GenerateRandomByteArray<Key>();
			
			auto notification = model::MosaicModifyLevyNotification<1>(
				test::UnresolveXor(Currency_Mosaic_Id), levy, owner);

			test::ObserveNotification(*pObserver, notification, context);
		}
	
		void RunTest(const NotifyMode& mode, LevySetupFunc action, TestResultFunc result) {
			
			ObserverTestContext context(mode, height);
			auto recipient = test::GenerateRandomByteArray<Address>();
			auto signer = test::GenerateRandomByteArray<Key>();
			auto oldLevyEntry = test::CreateValidMosaicLevy();
			
			auto levy = model::MosaicLevyRaw(model::LevyType::Percentile,
				test::UnresolveXor(recipient), test::UnresolveXor(Currency_Mosaic_Id), Amount(50));
			
			action(context.cache(), oldLevyEntry, signer);
			
			RunModifyTest(context, levy);
			
			result(context.cache(), levy, oldLevyEntry);
		}
		
		state::LevyEntry& GetLevyEntryFromCache(cache::CatapultCacheDelta& cache, const MosaicId& mosaicId) {
			auto& levyCache = cache.sub<cache::LevyCache>();
			auto iter = levyCache.find(mosaicId);
			return iter.get();
		}
	}
		
	TEST(TEST_CLASS, AddNewLevyCommitTest) {
		RunTest(
		NotifyMode::Commit,
		[](cache::CatapultCacheDelta& cache, const state::LevyEntryData&, const Key&){
			test::AddMosaic(cache, Currency_Mosaic_Id, Height(7),
				Eternal_Artifact_Duration, Amount(10000));
			},
			[](cache::CatapultCacheDelta& cache, const model::MosaicLevyRaw& levy, const state::LevyEntryData&) {
				auto result = GetLevyEntryFromCache(cache, Currency_Mosaic_Id);
				auto resolverContext = test::CreateResolverContextXor();
				
				test::AssertLevy(*result.levy(), levy, resolverContext);
		});
	}
		
	TEST(TEST_CLASS, RollbackLevyWithoutHistoryTest) {
		RunTest(
			NotifyMode::Rollback,
			[](cache::CatapultCacheDelta& cache, const state::LevyEntryData& levyEntry, const Key& signer){
				test::AddMosaicWithLevy(cache, Currency_Mosaic_Id, Height(1), levyEntry, signer);},
				[](cache::CatapultCacheDelta& cache, const model::MosaicLevyRaw&, const state::LevyEntryData&) {
					auto& levyCache = cache.sub<cache::LevyCache>();
					auto iter = levyCache.find(Currency_Mosaic_Id);
					
					EXPECT_EQ(iter.tryGet(), nullptr);
			});
	}
	
	TEST(TEST_CLASS, ModifyMosaicUpdateLevyCommit) {
		RunTest(
			NotifyMode::Commit,
			[](cache::CatapultCacheDelta& cache, const state::LevyEntryData& levyEntry, const Key& signer){
				test::AddMosaicWithLevy(cache, Currency_Mosaic_Id, Height(1), levyEntry, signer);},
			[](cache::CatapultCacheDelta& cache, const model::MosaicLevyRaw& levy, const state::LevyEntryData& oldEntry) {
				auto entry = GetLevyEntryFromCache(cache, Currency_Mosaic_Id);
				auto result = entry.levy();
				auto resolverContext = test::CreateResolverContextXor();
				
				test::AssertLevy(*result, levy, resolverContext);
				
				auto updateEntry = entry.updateHistories().at(height);
				test::AssertLevy(updateEntry, oldEntry);
			});
	}
	
	TEST(TEST_CLASS, ModifyMosaicUpdateLevyRollbackWithHistory) {
		auto historyData = state::LevyEntryData();
		historyData.Type = model::LevyType::Absolute;
		historyData.Recipient = test::GenerateRandomByteArray<Address>();
		historyData.MosaicId = MosaicId(1000);
		historyData.Fee = test::CreateMosaicLevyFeePercentile(10); // 10% levy fee
		
		RunTest(
			NotifyMode::Rollback,
			[&historyData](cache::CatapultCacheDelta& cache, const state::LevyEntryData& levyEntry, const Key& signer) {
				test::AddMosaicWithLevy(cache, Currency_Mosaic_Id, Height(1), levyEntry, signer);
				
				auto& entry = GetLevyEntryFromCache(cache, Currency_Mosaic_Id);
				entry.updateHistories().emplace(height, historyData);
				
				// add more history data
				for(auto i = 0; i < 10; i++) {
					auto history = test::CreateValidMosaicLevy();
					entry.updateHistories().emplace(Height(100+i), history);
				}
			},
			[&historyData](cache::CatapultCacheDelta& cache, const model::MosaicLevyRaw&, const state::LevyEntryData&) {
				auto& entry = GetLevyEntryFromCache(cache, Currency_Mosaic_Id);;
				test::AssertLevy(*entry.levy(), historyData);
			});
	}
}}