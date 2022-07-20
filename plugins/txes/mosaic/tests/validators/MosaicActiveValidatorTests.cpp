/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "src/validators/ActiveMosaicView.h"
#include "tests/test/MosaicCacheTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS MosaicActiveValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(MosaicActive, )

    TEST(TEST_CLASS, FailureWhenMosaicHasExpired) {
        // Arrange:
        auto cache = test::MosaicCacheFactory::Create();
        auto delta = cache.createDelta();
		test::AddMosaic(delta,  MosaicId(123), Height(50), BlockDuration(100), Amount());
		cache.commit(Height());
        auto pValidator = CreateMosaicActiveValidator();
        auto notification = model::MosaicActiveNotification<1>(test::UnresolveXor(MosaicId(123)), Height(200));

        // Act:
		auto result = test::ValidateNotification(*pValidator, notification, cache, config::BlockchainConfiguration::Uninitialized());

        // Assert:
        EXPECT_EQ(Failure_Mosaic_Expired, result);
    }

   TEST(TEST_CLASS, FailureWhenSdaOfferDurationExceedMosaicDuration) {
        // Arrange:
        auto cache = test::MosaicCacheFactory::Create();
        auto delta = cache.createDelta();
		test::AddMosaic(delta,  MosaicId(123), Height(50), BlockDuration(100), Amount());
		cache.commit(Height());
        auto pValidator = CreateMosaicActiveValidator();
        auto notification = model::MosaicActiveNotification<1>(test::UnresolveXor(MosaicId(123)), Height(151));

        // Act:
		auto result = test::ValidateNotification(*pValidator, notification, cache, config::BlockchainConfiguration::Uninitialized(), Height(100));

        // Assert:
        EXPECT_EQ(Failure_Mosaic_Offer_Duration_Exceeds_Mosaic_Duration, result);
    }

    TEST(TEST_CLASS, SuccessWhenSdaOfferDurationInsideMosaicDuration) {
        // Arrange:
        auto cache = test::MosaicCacheFactory::Create();
        auto delta = cache.createDelta();
		test::AddMosaic(delta,  MosaicId(123), Height(50), BlockDuration(100), Amount());
		cache.commit(Height());
        auto pValidator = CreateMosaicActiveValidator();
        auto notification = model::MosaicActiveNotification<1>(test::UnresolveXor(MosaicId(123)), Height(150));

        // Act:
		auto result = test::ValidateNotification(*pValidator, notification, cache, config::BlockchainConfiguration::Uninitialized(), Height(100));

        // Assert:
        EXPECT_EQ(ValidationResult::Success, result);
    }
}}