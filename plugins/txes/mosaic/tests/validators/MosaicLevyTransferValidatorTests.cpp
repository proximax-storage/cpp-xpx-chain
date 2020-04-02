#include "src/validators/Validators.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/MosaicCacheTestUtils.h"
#include "tests/test/cache/BalanceTransferTestUtils.h"
#include "plugins/txes/mosaic/src/model/MosaicLevy.h"
#include "catapult/types.h"
#include "src/model/MosaicEntityType.h"
#include "catapult/constants.h"
#include "tests/test/MosaicTestUtils.h"

namespace catapult {
	namespace validators {

#define TEST_CLASS MosaicLevyTransferValidatorTests
		
		DEFINE_COMMON_VALIDATOR_TESTS(MosaicLevyTransfer,)
		
		namespace {
			constexpr UnresolvedMosaicId unresMosaicId(1234);
			auto Currency_Mosaic_Id = MosaicId(unresMosaicId.unwrap());
			auto Levy_Mosaic_Id = MosaicId(444);
			
			void AddMosaicWithLevyWithOwner(cache::CatapultCacheDelta& delta, model::MosaicLevy& levy, const Key& owner)
			{
				test::AddMosaicWithLevy(delta, Currency_Mosaic_Id, Height(1), Eternal_Artifact_Duration, levy);
				test::AddMosaicOwner(delta, Currency_Mosaic_Id, owner, Amount(1000));
			}
			
			void AssertValidationResult(ValidationResult expectedResult, const Key &sender, UnresolvedAddress recipient,
			                            UnresolvedMosaicId mosaicId,
			                            Amount amount, cache::CatapultCache &cache) {
				// Arrange:
				utils::Mempool pool;
				auto pMosaicLevyData = pool.malloc(model::MosaicLevyData(mosaicId));
				auto unresolvedAmount = UnresolvedAmount(amount.unwrap(), UnresolvedAmountType::LeviedTransfer,
				                                         pMosaicLevyData);
				
				model::BalanceTransferNotification<1> notification(sender, recipient, mosaicId, unresolvedAmount);
				
				auto pValidator = CreateMosaicLevyTransferValidator();
				auto result = test::ValidateNotification(*pValidator, notification, cache, test::CreateMosaicResolverContextDefault());
				
				// Assert:
				EXPECT_EQ(expectedResult, result);
			}
		}
		
		// region validator tests
		
		TEST(TEST_CLASS, SuccessSameMosaicTransferLevy) {
			auto sender = test::GenerateRandomByteArray<Key>();
			auto recipient = test::GenerateRandomByteArray<UnresolvedAddress>();
			auto sink = test::GenerateRandomByteArray<UnresolvedAddress>();
			
			auto cache = test::MosaicCacheFactory::Create();
			auto delta = cache.createDelta();
			auto levy = model::MosaicLevy(model::LevyType::Absolute, sink, Currency_Mosaic_Id,Amount(10));
			
			AddMosaicWithLevyWithOwner(delta, levy, sender);
			cache.commit(Height());
			
			AssertValidationResult(ValidationResult::Success, sender, recipient, unresMosaicId, Amount(10), cache);
		}
		
		TEST(TEST_CLASS, MosaicIdNotFound) {
			auto sender = test::GenerateRandomByteArray<Key>();
			auto recipient = test::GenerateRandomByteArray<UnresolvedAddress>();
			
			auto cache = test::MosaicCacheFactory::Create();
			AssertValidationResult(Failure_Mosaic_Id_Not_Found, sender, recipient, unresMosaicId, Amount(10), cache);
		}
		
		TEST(TEST_CLASS, SinkRecipientNotFound) {
			auto sender = test::GenerateRandomByteArray<Key>();
			auto recipient = test::GenerateRandomByteArray<UnresolvedAddress>();
			
			auto cache = test::MosaicCacheFactory::Create();
			auto delta = cache.createDelta();
			auto levy = model::MosaicLevy(model::LevyType::Absolute, catapult::UnresolvedAddress(), Levy_Mosaic_Id, Amount(10));
			
			AddMosaicWithLevyWithOwner(delta, levy, sender);
			cache.commit(Height());
			
			AssertValidationResult(Failure_Mosaic_Recipient_Levy_Not_Exist, sender, recipient, unresMosaicId, Amount(10), cache);
		}
		
		TEST(TEST_CLASS, NotEnoughLevyMosaicBalanceOtherCurrency) {
			auto sender = test::GenerateRandomByteArray<Key>();
			auto recipient = test::GenerateRandomByteArray<UnresolvedAddress>();
			auto sink = test::GenerateRandomByteArray<UnresolvedAddress>();
			
			auto cache = test::MosaicCacheFactory::Create();
			auto delta = cache.createDelta();
			
			auto resolverContext = test::CreateMosaicResolverContextDefault();
			auto sinkResolved = resolverContext.resolve(sink);
			auto levy = model::MosaicLevy(model::LevyType::Absolute, sink, Levy_Mosaic_Id, Amount(10));
			
			// credit a balance to sink so it will appear in account cache
			test::SetCacheBalances(delta, sinkResolved, test::CreateMosaicBalance(Levy_Mosaic_Id, Amount(1)));
			test::AddMosaic(delta, Levy_Mosaic_Id, Height(1), Eternal_Artifact_Duration, Amount(100));
			
			AddMosaicWithLevyWithOwner(delta, levy, sender);
			cache.commit(Height());
			
			AssertValidationResult(Failure_Mosaic_Insufficient_Levy_Balance, sender, recipient, unresMosaicId, Amount(10), cache);
		}
		
		TEST(TEST_CLASS, SuccessLevyMosaicBalanceOtherCurrency) {
			auto sender = test::GenerateRandomByteArray<Key>();
			auto recipient = test::GenerateRandomByteArray<UnresolvedAddress>();
			auto sink = test::GenerateRandomByteArray<UnresolvedAddress>();
			
			auto cache = test::MosaicCacheFactory::Create();
			auto delta = cache.createDelta();
			
			auto resolverContext = test::CreateMosaicResolverContextDefault();
			auto sinkResolved = resolverContext.resolve(sink);
			auto levy = model::MosaicLevy(model::LevyType::Absolute, sink, Levy_Mosaic_Id, Amount(5));
			
			test::SetCacheBalances(delta, sinkResolved, test::CreateMosaicBalance(Levy_Mosaic_Id, Amount(1)));
			test::SetCacheBalances(delta, sender, test::CreateMosaicBalance(Levy_Mosaic_Id, Amount(10)));
			test::AddMosaic(delta, Levy_Mosaic_Id, Height(1), Eternal_Artifact_Duration, Amount(100));
			
			AddMosaicWithLevyWithOwner(delta, levy, sender);
			cache.commit(Height());
			
			AssertValidationResult(ValidationResult::Success, sender, recipient, unresMosaicId, Amount(10), cache);
		}
		
		// endregion
	}
}
