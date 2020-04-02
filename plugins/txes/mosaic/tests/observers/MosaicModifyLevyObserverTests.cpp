#include "src/observers/Observers.h"
#include "tests/test/cache/BalanceTransferTestUtils.h"
#include "tests/test/plugins/AccountObserverTestContext.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"
#include "catapult/utils/MemoryUtils.h"
#include "plugins/txes/mosaic/src/model/MosaicLevy.h"
#include "catapult/types.h"
#include "src/model/MosaicEntityType.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/test/MosaicCacheTestUtils.h"
#include "src/model/MosaicModifyLevyTransaction.h"

namespace catapult {
	namespace observers {

#define TEST_CLASS MosaicModifyLevyObserver

		DEFINE_COMMON_OBSERVER_TESTS(ModifyLevy, )

		namespace {
			using ObserverTestContext = test::ObserverTestContextT<test::MosaicCacheFactory>;

			constexpr UnresolvedMosaicId unresMosaicId(1234);
			auto Currency_Mosaic_Id = MosaicId(unresMosaicId.unwrap());

			void RunTest(ObserverTestContext& context) {
				auto pObserver = CreateModifyLevyObserver();

				auto owner = test::GenerateRandomByteArray<Key>();

				test::AddMosaicWithLevy(context.cache(), Currency_Mosaic_Id,Height(7), Eternal_Artifact_Duration,
						model::MosaicLevy(model::LevyType::Absolute, catapult::UnresolvedAddress(), Currency_Mosaic_Id,Amount(10)));

				test::AddMosaicOwner(context.cache(), Currency_Mosaic_Id, owner, Amount(100000));

				auto notification = model::MosaicModifyLevyNotification<1>(
						model::MosaicLevyModifyBitChangeType | model::MosaicLevyModifyBitChangeLevyFee, Currency_Mosaic_Id,
						model::MosaicLevy(model::LevyType::Percentile,catapult::UnresolvedAddress(),
								MosaicId(0), Amount(50)), owner);

				test::ObserveNotification(*pObserver, notification, context);
			}
		}

		TEST(TEST_CLASS, ModifyLevyCommitTest) {

			ObserverTestContext context(NotifyMode::Commit, Height{444});

			RunTest(context);

			auto& mosaicCache = context.cache().sub<cache::MosaicCache>();
			auto mosaicIter = mosaicCache.find(Currency_Mosaic_Id);
			auto& entry = mosaicIter.get();
			auto& levy = entry.levy();

			EXPECT_EQ(levy.Type, model::LevyType::Percentile);
			EXPECT_EQ(levy.Recipient, catapult::UnresolvedAddress());
			EXPECT_EQ(levy.MosaicId, Currency_Mosaic_Id);
			EXPECT_EQ(levy.Fee, Amount(50));
		}

		TEST(TEST_CLASS, ModifyLevyRollbackTest) {

			ObserverTestContext context(NotifyMode::Rollback, Height{444});

			RunTest(context);

			auto& mosaicCache = context.cache().sub<cache::MosaicCache>();
			auto mosaicIter = mosaicCache.find(Currency_Mosaic_Id);
			auto& entry = mosaicIter.get();
			auto& levy = entry.levy();

			EXPECT_EQ(levy.Type, model::LevyType::Absolute);
			EXPECT_EQ(levy.Recipient, catapult::UnresolvedAddress());
			EXPECT_EQ(levy.MosaicId, Currency_Mosaic_Id);
			EXPECT_EQ(levy.Fee, Amount(10));
		}
	}
}