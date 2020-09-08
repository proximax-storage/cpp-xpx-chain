/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/MosaicPlugin.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/TestHarness.h"
#include "tests/test/LevyTestUtils.h"

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
	}

#define TEST_CLASS MosaicPluginResolversTest

	TEST(TEST_CLASS, ResolveMosaicLevy) {
		auto manager = SetUpPluginManager();
			
		auto cache = manager.createCache();
		auto delta = cache.createDelta();
		
		auto signer = test::GenerateRandomByteArray<Key>();
		auto oldLevyEntry = test::CreateValidMosaicLevy();
		
		auto Unresolved_Mosaic_Id = UnresolvedMosaicId(1234);
		auto Currency_Mosaic_Id = MosaicId(Unresolved_Mosaic_Id.unwrap());
		
		// Create mosaic with levy
		test::AddMosaicWithLevy(delta, Currency_Mosaic_Id, Height(1), oldLevyEntry, signer);
		cache.commit(Height());
		
		/// create unresolved data
		utils::Mempool pool;
		auto pMosaicLevyData = pool.malloc(model::MosaicLevyData(Unresolved_Mosaic_Id));
		auto unresolvedMosaicId = catapult::UnresolvedLevyMosaicId(Unresolved_Mosaic_Id.unwrap());
		auto unresolvedAmount = UnresolvedAmount(Amount(2000000), UnresolvedAmountType::MosaicLevy, pMosaicLevyData);
		auto unresolvedAddress = catapult::UnresolvedLevyAddress(test::GenerateRandomByteArray<UnresolvedAddress>().array(),
		    catapult::UnresolvedCommonType::MosaicLevy, pMosaicLevyData);
		
		auto cacheView = cache.createView();
		auto readOnly = cacheView.toReadOnly();
		auto resolverContext = manager.createResolverContext(readOnly);
		
		auto levyMosaicId = resolverContext.resolve(unresolvedMosaicId);
		auto recipient = resolverContext.resolve(unresolvedAddress);
		auto amount = resolverContext.resolve(unresolvedAmount);
		
		EXPECT_EQ(levyMosaicId, oldLevyEntry.MosaicId);
		EXPECT_EQ(recipient, oldLevyEntry.Recipient);
		EXPECT_EQ(amount, oldLevyEntry.Fee);
	}
	
	TEST(TEST_CLASS, ResolveMosaicLevyAssertNull) {
		auto manager = SetUpPluginManager();
		
		auto unresolvedAmount = UnresolvedAmount(Amount(100), UnresolvedAmountType::MosaicLevy, nullptr);
		auto unresolvedAddress = catapult::UnresolvedLevyAddress(test::GenerateRandomByteArray<UnresolvedAddress>().array(),
			catapult::UnresolvedCommonType::MosaicLevy, nullptr);
		
		auto cache = manager.createCache();
		auto cacheView = cache.createView();
		auto readOnly = cacheView.toReadOnly();
		auto resolverContext = manager.createResolverContext(readOnly);
		
		EXPECT_THROW(resolverContext.resolve(unresolvedAmount), catapult_runtime_error);
		EXPECT_THROW(resolverContext.resolve(unresolvedAddress), catapult_runtime_error);
	}
}}
