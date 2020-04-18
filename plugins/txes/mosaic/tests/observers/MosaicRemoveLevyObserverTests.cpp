/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
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
#include "src/model/MosaicRemoveLevyTransaction.h"
#include "src/cache/LevyCache.h"
namespace catapult {
	namespace observers {

#define TEST_CLASS MosaicRemoveLevyObserver
		
		using ObserverTestContext = test::ObserverTestContextT<test::LevyCacheFactory>;
		
		/// region Add levy
		DEFINE_COMMON_OBSERVER_TESTS(RemoveLevy, )

		namespace {
			void RunTest(ObserverTestContext& context, const MosaicId& id, const Key& signer ) {
				// Arrange:
				auto pObserver = CreateRemoveLevyObserver();
				
				// Act
				auto notification = model::MosaicRemoveLevyNotification<1>(id, signer);

				// Trigger
				test::ObserveNotification(*pObserver, notification, context);
			}
			
		}
		
		TEST(TEST_CLASS, RemoveMosaicLevyCommit) {
			
			ObserverTestContext context(NotifyMode::Commit, Height{444});
			
			auto signer = test::GenerateRandomByteArray<Key>();
			auto levy = test::CreateValidMosaicLevy();
			auto mosaicId = MosaicId(123);
			
			test::AddMosaicWithLevy(context.cache(), mosaicId, Height(1), levy, signer);
			
			RunTest(context, mosaicId, signer);
			
			auto& levyCache = context.cache().sub<cache::LevyCache>();
			auto iter = levyCache.find(mosaicId);
			
			EXPECT_EQ(iter.tryGet(), nullptr);
		}
		
		/// end region
		
	}
}