/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/MosaicCacheTestUtils.h"
#include "catapult/plugins/PluginUtils.h"
#include "src/plugins/MosaicPlugin.h"
#include "src/cache/MosaicCache.h"
#include "src/model/MosaicEntityType.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/TestHarness.h"
#include "tests/test/LevyTestUtils.h"
#include "catapult/types.h"

namespace catapult { namespace plugins {

	namespace {
		plugins::PluginManager SetUpPluginManager() {
			// Arrange:
			auto config = model::NetworkConfiguration::Uninitialized();
			config.BlockGenerationTargetTime = utils::TimeSpan::FromSeconds(1);
			config.BlockPruneInterval = 150;
			config.Plugins.emplace(PLUGIN_NAME(mosaic), utils::ConfigurationBag({{
                 "",
                 {
                     { "maxMosaicsPerAccount", "0" },
                     { "maxMosaicDuration", "0h" },
                     { "maxMosaicDivisibility", "0" },

                     { "mosaicRentalFeeSinkPublicKey", "0000000000000000000000000000000000000000000000000000000000000000" },
                     { "mosaicRentalFee", "0" }
                 }
             }}));
			
			auto manager = test::CreatePluginManager(config);
			RegisterMosaicSubsystem(manager);
			
			return manager;
		}
		
		using SetUpFunc = std::function<void(cache::CatapultCache&, cache::CatapultCacheDelta&, Key&)>;
		using ResultFunc = std::function<void(model::ResolverContext&)>;
		auto Unresolved_Mosaic_Id = UnresolvedMosaicId(1234);
		auto Currency_Mosaic_Id = MosaicId(Unresolved_Mosaic_Id.unwrap());
		
		void RunTest(SetUpFunc setup, ResultFunc result) {
			
			auto manager = SetUpPluginManager();
			
			auto cache = manager.createCache();
			auto delta = cache.createDelta();
			
			auto signer = test::GenerateRandomByteArray<Key>();
			
			setup(cache, delta, signer);
			
			auto cacheView = cache.createView();
			auto readOnly = cacheView.toReadOnly();
			auto resolverContext = manager.createResolverContext(readOnly);
			
			result(resolverContext);
		}
	}

#define TEST_CLASS MosaicPluginResolversTest

	TEST(TEST_CLASS, ResolveMosaicLevy) {
			
		auto oldLevyEntry = test::CreateValidMosaicLevy();
		
		RunTest( [oldLevyEntry](cache::CatapultCache& cache, cache::CatapultCacheDelta& delta, Key& signer){
			
			test::AddMosaicWithLevy(delta, Currency_Mosaic_Id, Height(1), oldLevyEntry, signer);
			cache.commit(Height());
			
		}, [oldLevyEntry](model::ResolverContext& resolverContext){
			/// create unresolved data
			utils::Mempool pool;
			auto pMosaicLevyData = pool.malloc(model::MosaicLevyData(Unresolved_Mosaic_Id));
			auto unresolvedMosaicId = catapult::UnresolvedLevyMosaicId(Unresolved_Mosaic_Id.unwrap());
			auto unresolvedAmount = UnresolvedAmount(Amount(2000000), UnresolvedAmountType::MosaicLevy, pMosaicLevyData);
			auto unresolvedAddress = catapult::UnresolvedLevyAddress(test::GenerateRandomByteArray<UnresolvedAddress>().array(),
			                                                         catapult::UnresolvedCommonType::MosaicLevy, pMosaicLevyData);
			
			auto levyMosaicId = resolverContext.resolve(unresolvedMosaicId);
			auto recipient = resolverContext.resolve(unresolvedAddress);
			auto amount = resolverContext.resolve(unresolvedAmount);
			
			EXPECT_EQ(levyMosaicId, oldLevyEntry.MosaicId);
			EXPECT_EQ(recipient, oldLevyEntry.Recipient);
			EXPECT_EQ(amount, oldLevyEntry.Fee);
		});
	}
	
	TEST(TEST_CLASS, ResolveMosaicLevyAssertNull) {
		RunTest( [](cache::CatapultCache&,cache::CatapultCacheDelta&, Key&){
			// empty no setup needed
		}, [](model::ResolverContext& resolverContext) {
			auto unresolvedAmount = UnresolvedAmount(Amount(100), UnresolvedAmountType::MosaicLevy, nullptr);
			auto unresolvedAddress = catapult::UnresolvedLevyAddress(test::GenerateRandomByteArray<UnresolvedAddress>().array(),
			                                                         catapult::UnresolvedCommonType::MosaicLevy, nullptr);
			
			EXPECT_THROW(resolverContext.resolve(unresolvedAmount), catapult_runtime_error);
			EXPECT_THROW(resolverContext.resolve(unresolvedAddress), catapult_runtime_error);
		});
		
	}
	
	TEST(TEST_CLASS, ResolveMosaicIdWithoutLevy) {
		RunTest( [](cache::CatapultCache& cache, cache::CatapultCacheDelta& delta, Key& signer){
			// create a base mosaic without levy
			test::AddMosaic(delta, Currency_Mosaic_Id, Height(1), Amount(100), signer);
			cache.commit(Height());
			
		}, [](model::ResolverContext& resolverContext) {
			auto unresolvedMosaicId = catapult::UnresolvedLevyMosaicId(Unresolved_Mosaic_Id.unwrap());
			auto levyMosaicId = resolverContext.resolve(unresolvedMosaicId);
			EXPECT_EQ(levyMosaicId, Currency_Mosaic_Id);
		});
	}
	
	TEST(TEST_CLASS, ResolveMosaicIdWithLevyButDeleted) {

		auto oldLevyEntry = test::CreateValidMosaicLevy();
		
		RunTest( [oldLevyEntry](cache::CatapultCache& cache, cache::CatapultCacheDelta& delta, Key& signer){
			
			// Create mosaic with levy but deleted
			test::AddMosaicWithLevy(delta, Currency_Mosaic_Id, Height(1), oldLevyEntry, signer);
			auto& levyCacheDelta = delta.sub<cache::LevyCache>();
			auto entry = levyCacheDelta.find(Currency_Mosaic_Id);
			entry.get().remove(Height(2));
			cache.commit(Height());
			
		}, [oldLevyEntry](model::ResolverContext& resolverContext) {
			auto unresolvedMosaicId = catapult::UnresolvedLevyMosaicId(Unresolved_Mosaic_Id.unwrap());
			auto levyMosaicId = resolverContext.resolve(unresolvedMosaicId);
			EXPECT_EQ(levyMosaicId, Currency_Mosaic_Id);
		});
	}
}}
