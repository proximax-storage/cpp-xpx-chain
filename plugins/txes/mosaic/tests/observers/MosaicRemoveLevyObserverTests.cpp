/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
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

#define TEST_CLASS MosaicRemoveLevyObserver
		
	using ObserverTestContext = test::ObserverTestContextT<test::LevyCacheFactory>;
	
	/// region remove levy
	DEFINE_COMMON_OBSERVER_TESTS(RemoveLevy, )

	namespace {
		Height height(444);
		using LevySetupFunc = std::function<void (cache::CatapultCacheDelta&, const state::LevyEntryData&, const MosaicId& mosaicId, const Key&)>;
		using TestResultFunc = std::function<void (cache::CatapultCacheDelta&, const MosaicId& mosaicId)>;
		
		void RunTest(ObserverTestContext& context, const UnresolvedMosaicId& id, const Key& signer ) {
			// Arrange:
			auto pObserver = CreateRemoveLevyObserver();
			
			// Act
			auto notification = model::MosaicRemoveLevyNotification<1>(id, signer);

			// Trigger
			test::ObserveNotification(*pObserver, notification, context);
		}
		
		void RunTest(const NotifyMode& mode, LevySetupFunc action, TestResultFunc result) {
			
			ObserverTestContext context(mode, height);
			
			auto signer = test::GenerateRandomByteArray<Key>();
			auto levy = test::CreateValidMosaicLevy();
			auto unresolvedMosaicId = UnresolvedMosaicId (123);
			auto mosaicId = MosaicId(unresolvedMosaicId.unwrap());
			
			action(context.cache(), levy, mosaicId, signer);
			
			RunTest(context,  test::UnresolveXor(mosaicId), signer);
			
			result(context.cache(), mosaicId);
		}
	}
	
	TEST(TEST_CLASS, RemoveMosaicLevyCommit) {
			
		RunTest(NotifyMode::Commit,[](cache::CatapultCacheDelta& cache, const state::LevyEntryData& levy,
			const MosaicId& mosaicId, const Key& signer){
			test::AddMosaicWithLevy(cache, mosaicId, Height(1), levy, signer);
		},[](cache::CatapultCacheDelta& cache, const MosaicId& mosaicId){
			auto& entry = test::GetLevyEntryFromCache(cache, mosaicId);
			EXPECT_EQ(entry.levy(), nullptr);
		
		});
	}
	
	TEST(TEST_CLASS, RemoveMosaicLevyRollback) {
		auto historyData = state::LevyEntryData();
		historyData.Type = model::LevyType::Absolute;
		historyData.Recipient = test::GenerateRandomByteArray<Address>();
		historyData.MosaicId = MosaicId(1000);
		historyData.Fee = test::CreateMosaicLevyFeePercentile(10);
		
		RunTest(NotifyMode::Rollback,[&historyData](cache::CatapultCacheDelta& cache, const state::LevyEntryData& levy,
		                              const MosaicId& mosaicId, const Key& signer){
			
			test::AddMosaicWithLevy(cache, mosaicId, Height(1), levy, signer);
			
			auto& entry = test::GetLevyEntryFromCache(cache, mosaicId);
			entry.updateHistory().push_back(std::make_pair(height, std::move(historyData)));
			
		},[&historyData](cache::CatapultCacheDelta& cache, const MosaicId& mosaicId){
			
			auto resolverContext = test::CreateResolverContextXor();
			auto result = test::GetLevyEntryFromCache(cache, mosaicId).levy();
			
			test::AssertLevy(*result, historyData);
			
		});
	}
	/// end region
}}