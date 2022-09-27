/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "plugins/txes/storage/src/validators/Validators.h"
#include "tests/test/StorageTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS ServiceUnitTransferValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(ServiceUnitTransfer, )

	namespace {
		constexpr MosaicId Non_Service_Mosaic_Id {0};
		constexpr MosaicId Storage_Mosaic_Id {1};
		constexpr MosaicId Streaming_Mosaic_Id {2};
		constexpr MosaicId Review_Mosaic_Id {3};
		constexpr MosaicId Super_Contract_Mosaic_Id {4};
		constexpr MosaicId Harvesting_Mosaic_Id {5};
		constexpr MosaicId Xar_Mosaic_Id {6};

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;

			config.Immutable.StorageMosaicId = Storage_Mosaic_Id;
			config.Immutable.StreamingMosaicId = Streaming_Mosaic_Id;
			config.Immutable.ReviewMosaicId = Review_Mosaic_Id;
			config.Immutable.SuperContractMosaicId = Super_Contract_Mosaic_Id;
			config.Immutable.HarvestingMosaicId = Harvesting_Mosaic_Id;
			config.Immutable.XarMosaicId = Xar_Mosaic_Id;

			return config.ToConst();
		}

		void AssertValidationResult(
				ValidationResult expectedResult,
				const MosaicId& mosaicId) {
			// Arrange:
			auto cache = test::StorageCacheFactory::Create();
			auto unresolvedMosaicId = test::UnresolveXor(mosaicId);
			auto notification = model::BalanceTransferNotification<1>(Key(), UnresolvedAddress(), unresolvedMosaicId, Amount(123));
			auto pValidator = CreateServiceUnitTransferValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, CreateConfig());

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, SuccessWhenTransferringNonServiceMosaic) {
		AssertValidationResult(ValidationResult::Success, Non_Service_Mosaic_Id);
	}

	TEST(TEST_CLASS, FailureWhenTransferringStorageMosaic) {
		AssertValidationResult(Failure_Storage_Service_Unit_Transfer, Storage_Mosaic_Id);
	}

	TEST(TEST_CLASS, FailureWhenTransferringStreamingMosaic) {
		AssertValidationResult(Failure_Storage_Service_Unit_Transfer, Streaming_Mosaic_Id);
	}

	TEST(TEST_CLASS, FailureWhenTransferringReviewMosaic) {
		AssertValidationResult(Failure_Storage_Service_Unit_Transfer, Review_Mosaic_Id);
	}

	TEST(TEST_CLASS, FailureWhenTransferringSuperContractMosaic) {
		AssertValidationResult(Failure_Storage_Service_Unit_Transfer, Super_Contract_Mosaic_Id);
	}
}}
