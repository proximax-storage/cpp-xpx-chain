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
#include "tests/test/MosaicCacheTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS ProperMosaicValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(ProperMosaic,)

	namespace {
		constexpr auto Mosaic_Expiry_Height = Height(150);

		struct ResolvedMosaicTraits {
			static constexpr auto Default_Id = MosaicId(110);
		};

		struct UnresolvedMosaicTraits {
			// custom resolver doubles unresolved mosaic ids
			static constexpr auto Default_Id = UnresolvedMosaicId(55);
		};

		template<typename TMosaicId>
		void AssertValidationResult(
				ValidationResult expectedResult,
				TMosaicId affectedMosaicId,
				Height height,
				const Key& transactionSigner,
				const Key& artifactOwner) {
			// Arrange:
			auto pValidator = CreateProperMosaicValidator();

			// - create the notification
			model::MosaicRequiredNotification<1> notification(transactionSigner, affectedMosaicId);

			// - create the validator context
			auto cache = test::MosaicCacheFactory::Create();
			auto delta = cache.createDelta();
			test::AddMosaic(delta, ResolvedMosaicTraits::Default_Id, Height(50), BlockDuration(100), artifactOwner);
			cache.commit(Height());

			auto readOnlyCache = delta.toReadOnly();
			auto config = config::BlockchainConfiguration::Uninitialized();
			auto context = test::CreateValidatorContext(config, height, readOnlyCache);

			// - set up a custom mosaic id resolver
			const_cast<model::ResolverContext&>(context.Resolvers) = test::CreateResolverContextWithCustomDoublingMosaicResolver();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, context);

			// Assert:
			EXPECT_EQ(expectedResult, result) << "height " << height << ", id " << affectedMosaicId;
		}

		template<typename TMosaicId>
		void AssertValidationResult(ValidationResult expectedResult, TMosaicId affectedMosaicId, Height height) {
			auto key = test::GenerateRandomByteArray<Key>();
			AssertValidationResult(expectedResult, affectedMosaicId, height, key, key);
		}
	}

#define MOSAIC_ID_TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Resolved) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ResolvedMosaicTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Unresolved) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<UnresolvedMosaicTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	MOSAIC_ID_TRAITS_BASED_TEST(FailureWhenMosaicIsUnknown) {
		// Assert:
		auto unknownMosaicId = TTraits::Default_Id + decltype(TTraits::Default_Id)(1);
		AssertValidationResult(Failure_Mosaic_Expired, unknownMosaicId, Height(100));
	}

	MOSAIC_ID_TRAITS_BASED_TEST(FailureWhenMosaicExpired) {
		// Assert:
		AssertValidationResult(Failure_Mosaic_Expired, TTraits::Default_Id, Mosaic_Expiry_Height);
	}

	MOSAIC_ID_TRAITS_BASED_TEST(FailureWhenMosaicOwnerDoesNotMatch) {
		// Assert:
		auto key1 = test::GenerateRandomByteArray<Key>();
		auto key2 = test::GenerateRandomByteArray<Key>();
		AssertValidationResult(Failure_Mosaic_Owner_Conflict, TTraits::Default_Id, Height(100), key1, key2);
	}

	MOSAIC_ID_TRAITS_BASED_TEST(SuccessWhenMosaicIsActiveAndOwnerMatches) {
		// Assert:
		AssertValidationResult(ValidationResult::Success, TTraits::Default_Id, Height(100));
	}
}}
