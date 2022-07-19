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

    namespace {
        constexpr MosaicId Default_Mosaic_Id = MosaicId(0x1234);

        model::MosaicProperties CreateMosaicPropertiesWithDuration(BlockDuration duration) {
            model::MosaicProperties::PropertyValuesContainer values{};
            values[utils::to_underlying_type(model::MosaicPropertyId::Duration)] = duration.unwrap();
            return model::MosaicProperties::FromValues(values);
        }

        void AddMosaic(cache::CatapultCacheDelta& cache, MosaicId id, Height height, BlockDuration duration, Amount supply) {
            auto& mosaicCacheDelta = cache.sub<cache::MosaicCache>();
            auto definition = state::MosaicDefinition(height, Key(), 1, CreateMosaicPropertiesWithDuration(duration));
            auto entry = state::MosaicEntry(id, definition);
            entry.increaseSupply(supply);
            mosaicCacheDelta.insert(entry);
        }
    }

   TEST(TEST_CLASS, FailureWhenSdaOfferDurationExceedMosaicDuration) {
        // Arrange:
        auto cache = test::MosaicCacheFactory::Create();
		auto delta = cache.createDelta();
		test::AddMosaic(delta, MosaicId(123), Height(50), BlockDuration(100), Amount());
		cache.commit(Height());
        auto pValidator = CreateMosaicActiveValidator();
        auto notification = model::MosaicActiveNotification<1>(test::UnresolveXor(MosaicId(123)), Height(200));

        // Act:
		auto result = test::ValidateNotification(*pValidator, notification);

        // Assert:
        EXPECT_EQ(Failure_Mosaic_Offer_Duration_Exceeds_Mosaic_Duration, result);
    }

    TEST(TEST_CLASS, SuccessWhenSdaOfferDurationInsideMosaicDuration) {
        // Arrange:
        auto cache = test::MosaicCacheFactory::Create();
		auto delta = cache.createDelta();
		test::AddMosaic(delta, MosaicId(123), Height(50), BlockDuration(100), Amount());
		cache.commit(Height());
        auto pValidator = CreateMosaicActiveValidator();
        auto notification = model::MosaicActiveNotification<1>(test::UnresolveXor(MosaicId(123)), Height(100));

        // Act:
		auto result = test::ValidateNotification(*pValidator, notification);

        // Assert:
        EXPECT_EQ(ValidationResult::Success, result);
    }
}}