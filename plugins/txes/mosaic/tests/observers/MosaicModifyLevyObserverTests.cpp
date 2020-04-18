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
		
		using ObserverTestContext = test::ObserverTestContextT<test::LevyCacheFactory>;
		constexpr UnresolvedMosaicId unresMosaicId(1234);
		auto Currency_Mosaic_Id = MosaicId(unresMosaicId.unwrap());
		
		/// region Add levy
		DEFINE_COMMON_OBSERVER_TESTS(AddLevy, )
		DEFINE_COMMON_OBSERVER_TESTS(UpdateLevy, )
		
		namespace {
			void RunAddTest(ObserverTestContext& context) {
				auto pObserver = CreateAddLevyObserver();

				auto owner = test::GenerateRandomByteArray<Key>();

				test::AddMosaic(context.cache(), Currency_Mosaic_Id, Height(7),
					Eternal_Artifact_Duration, Amount(10000));
				
				test::AddMosaicOwner(context.cache(), Currency_Mosaic_Id, owner, Amount(100000));
				
				auto notification = model::MosaicAddLevyNotification<1>(
						Currency_Mosaic_Id,
						model::MosaicLevy(model::LevyType::Percentile,catapult::UnresolvedAddress(),
								MosaicId(0), Amount(50)), owner);

				test::ObserveNotification(*pObserver, notification, context);
			}
			
			void RunUpdateTest(ObserverTestContext& context) {
				auto pObserver = CreateUpdateLevyObserver();
				
				auto owner = test::GenerateRandomByteArray<Key>();
				auto notification = model::MosaicUpdateLevyNotification<1>(
					model::MosaicLevyModifyBitChangeType | model::MosaicLevyModifyBitChangeLevyFee,
					Currency_Mosaic_Id,
					model::MosaicLevy(model::LevyType::Absolute,catapult::UnresolvedAddress(),
						MosaicId(0), Amount(10)), owner);
				
				test::ObserveNotification(*pObserver, notification, context);
			}
		}

		TEST(TEST_CLASS, ModifyMosaicAddLevyCommit) {

			ObserverTestContext context(NotifyMode::Commit, Height{444});
			
			RunAddTest(context);

			auto& levyCache = context.cache().sub<cache::LevyCache>();
			auto iter = levyCache.find(Currency_Mosaic_Id);
			auto& entry = iter.get();
			auto& levy = entry.levy();

			EXPECT_EQ(levy.Type, model::LevyType::Percentile);
			EXPECT_EQ(levy.Recipient, catapult::UnresolvedAddress());
			EXPECT_EQ(levy.MosaicId, MosaicId(0));
			EXPECT_EQ(levy.Fee, Amount(50));
		}

		TEST(TEST_CLASS, ModifyLevyRollbackTest) {

			ObserverTestContext context(NotifyMode::Rollback, Height{444});
			
			RunAddTest(context);

			auto& levyCache = context.cache().sub<cache::LevyCache>();
			auto iter = levyCache.find(Currency_Mosaic_Id);
			
			EXPECT_EQ(iter.tryGet(), nullptr);
		}
		
		/// end region
		
		/// start region Update levy
		TEST(TEST_CLASS, UpdateLevyInformation) {
			
			ObserverTestContext context(NotifyMode::Commit, Height{444});
			
			/// run add observer first to add levy to cache
			RunAddTest(context);
			
			// update levy information
			RunUpdateTest(context);
			
			auto& levyCache = context.cache().sub<cache::LevyCache>();
			auto iter = levyCache.find(Currency_Mosaic_Id);
			auto& entry = iter.get();
			auto& levy = entry.levy();
			
			EXPECT_EQ(levy.Type, model::LevyType::Absolute);
			EXPECT_EQ(levy.Recipient, catapult::UnresolvedAddress());
			EXPECT_EQ(levy.MosaicId, MosaicId(0));
			EXPECT_EQ(levy.Fee, Amount(10));
		}
		/// end region
	}
}