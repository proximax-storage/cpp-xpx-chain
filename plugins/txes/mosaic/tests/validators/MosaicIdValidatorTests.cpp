/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "src/validators/Validators.h"
#include "src/model/MosaicIdGenerator.h"
#include "catapult/utils/IntegerMath.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS MosaicIdValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(MosaicId,)

	TEST(TEST_CLASS, FailureWhenValidatingInvalidMosaicId) {
		// Arrange:
		auto signer = test::GenerateRandomByteArray<Key>();
		auto pValidator = CreateMosaicIdValidator();
		auto notification = model::MosaicNonceNotification<1>(signer, MosaicNonce(), MosaicId());

		// Act:
		auto result = test::ValidateNotification(*pValidator, notification);

		// Assert:
		EXPECT_EQ(Failure_Mosaic_Invalid_Id, result);
	}

	// region nonce and id consistency

	namespace {
		auto CreateMosaicNonceIdNotification(const Key& signer) {
			auto nonce = test::GenerateRandomValue<MosaicNonce>();
			auto mosaicId = model::GenerateMosaicId(signer, nonce);
			return model::MosaicNonceNotification<1>(signer, nonce, mosaicId);
		}
	}

	TEST(TEST_CLASS, SuccessWhenValidatingNotificationWithMatchingId) {
		// Arrange: note that CreateMosaicNonceIdNotification creates proper mosaic id
		auto signer = test::GenerateRandomByteArray<Key>();
		auto pValidator = CreateMosaicIdValidator();
		auto notification = CreateMosaicNonceIdNotification(signer);

		// Act:
		auto result = test::ValidateNotification(*pValidator, notification);

		// Assert:
		EXPECT_EQ(ValidationResult::Success, result);
	}

	TEST(TEST_CLASS, FailureWhenValidatingNotificationWithMismatchedId) {
		// Arrange:
		auto signer = test::GenerateRandomByteArray<Key>();
		auto pValidator = CreateMosaicIdValidator();

		for (auto i = 0u; i < utils::GetNumBits<uint64_t>(); ++i) {
			// - note that CreateMosaicNonceIdNotification creates proper mosaic id
			auto notification = CreateMosaicNonceIdNotification(signer);
			auto mutatedId = notification.MosaicId.unwrap() ^ (1ull << i);
			notification.MosaicId = MosaicId(mutatedId);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification);

			// Assert:
			EXPECT_EQ(Failure_Mosaic_Id_Mismatch, result) << i;
		}
	}

	// endregion
}}
