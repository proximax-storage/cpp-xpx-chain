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
#include "catapult/model/NetworkConfiguration.h"
#include "tests/test/MosaicCacheTestUtils.h"
#include "tests/test/core/ResolverTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS ProperMosaicValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(ProperMosaicV1,)
	DEFINE_COMMON_VALIDATOR_TESTS(ProperMosaicV2,)

	namespace {

		struct V1TestTraits
		{
			using Notification = model::MosaicRequiredNotification<1>;
			static stateful::NotificationValidatorPointerT<Notification> Create(){
				return CreateProperMosaicV1Validator();
			}
			template<typename TMosaicId>
			static Notification CreateNotification(const Key& key, const TMosaicId& mosaicId, model::MosaicRequirementAction){
				return Notification(key, mosaicId);
			}
		};

		struct V2TestTraits
		{
			using Notification = model::MosaicRequiredNotification<2>;
			static stateful::NotificationValidatorPointerT<Notification> Create(){
				return CreateProperMosaicV2Validator();
			}
			template<typename TMosaicId>
			static Notification CreateNotification(const Key& key, const TMosaicId& mosaicId, model::MosaicRequirementAction action ){
				return Notification(key, mosaicId, action);
			}
		};

		constexpr auto Mosaic_Expiry_Height = Height(150);

		struct ResolvedMosaicTraits {
			static constexpr auto Default_Id = MosaicId(110);
		};

		struct UnresolvedMosaicTraits {
			// custom resolver doubles unresolved mosaic ids
			static constexpr auto Default_Id = UnresolvedMosaicId(55);
		};

		template<typename TTestTraits, typename TMosaicId>
		void AssertValidationResult(
				ValidationResult expectedResult,
				TMosaicId affectedMosaicId,
				Height height,
				const Key& transactionSigner,
				const Key& artifactOwner) {
			// Arrange:
			auto pValidator = TTestTraits::Create();

			// - create the notification
			typename TTestTraits::Notification notification = TTestTraits::CreateNotification(transactionSigner, affectedMosaicId, model::MosaicRequirementAction::Set);

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

		template<typename TTestTraits, typename TMosaicId>
		void AssertValidationResult(ValidationResult expectedResult, TMosaicId affectedMosaicId, Height height) {
			auto key = test::GenerateRandomByteArray<Key>();
			AssertValidationResult<TTestTraits>(expectedResult, affectedMosaicId, height, key, key);
		}
	}

#define MOSAIC_ID_TRAITS_BASED_DUAL_TEST(TEST_NAME) \
	template<typename TTestTraits, typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Resolved_V1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<V1TestTraits, ResolvedMosaicTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Resolved_V2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<V2TestTraits, ResolvedMosaicTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Unresolved_V1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<V1TestTraits, UnresolvedMosaicTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Unresolved_V2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<V2TestTraits, UnresolvedMosaicTraits>(); } \
	template<typename TTestTraits, typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()



	MOSAIC_ID_TRAITS_BASED_DUAL_TEST(FailureWhenMosaicIsUnknown) {
		// Assert:
		auto unknownMosaicId = TTraits::Default_Id + decltype(TTraits::Default_Id)(1);
		AssertValidationResult<TTestTraits>(Failure_Mosaic_Expired, unknownMosaicId, Height(100));
	}

	MOSAIC_ID_TRAITS_BASED_DUAL_TEST(FailureWhenMosaicExpired) {
		// Assert:
		AssertValidationResult<TTestTraits>(Failure_Mosaic_Expired, TTraits::Default_Id, Mosaic_Expiry_Height);
	}

	MOSAIC_ID_TRAITS_BASED_DUAL_TEST(FailureWhenMosaicOwnerDoesNotMatch) {
		// Assert:
		auto key1 = test::GenerateRandomByteArray<Key>();
		auto key2 = test::GenerateRandomByteArray<Key>();
		AssertValidationResult<TTestTraits>(Failure_Mosaic_Owner_Conflict, TTraits::Default_Id, Height(100), key1, key2);
	}

	MOSAIC_ID_TRAITS_BASED_DUAL_TEST(SuccessWhenMosaicIsActiveAndOwnerMatches) {
		// Assert:
		AssertValidationResult<TTestTraits>(ValidationResult::Success, TTraits::Default_Id, Height(100));
	}
	// region nonzero property mask

	namespace {
		model::MosaicProperties CreateProperties(
				model::MosaicFlags flags,
				uint8_t divisibility,
				BlockDuration duration) {
			model::MosaicProperties::PropertyValuesContainer values;
			for (auto i = 0u; i < values.size(); ++i)
				values[i] = 0xDEADBEAF;

			values[0] = utils::to_underlying_type(flags);
			values[1] = divisibility;
			values[2] = duration.unwrap();


			return model::MosaicProperties::FromValues(values);
		}
		template<typename TMosaicId>
		void AssertPropertyFlagMaskValidationResult(
				ValidationResult expectedResult,
				TMosaicId affectedMosaicId,
				uint8_t notificationPropertyFlagMask,
				uint8_t mosaicPropertyFlagMask,
				model::MosaicRequirementAction action) {
			// Arrange:
			auto pValidator = CreateProperMosaicV2Validator();

			// - create the notification
			auto owner = test::GenerateRandomByteArray<Key>();
			model::MosaicRequiredNotification<2> notification(owner, affectedMosaicId, action, notificationPropertyFlagMask);

			// - create the validator context
			auto height = Height(50);
			auto config = config::BlockchainConfiguration::Uninitialized();
			auto cache = test::MosaicCacheFactory::Create(config);
			auto delta = cache.createDelta();

			{
				// need to set custom property flags, so can't use regular helpers (e.g. test::AddMosaic)
				auto& mosaicCacheDelta = delta.sub<cache::MosaicCache>();

				model::MosaicProperties properties = CreateProperties(static_cast<model::MosaicFlags>(mosaicPropertyFlagMask), 0, BlockDuration(100));
				auto definition = state::MosaicDefinition(height, owner, 1, properties);
				mosaicCacheDelta.insert(state::MosaicEntry(ResolvedMosaicTraits::Default_Id, definition));
			}

			auto readOnlyCache = delta.toReadOnly();
			auto context = test::CreateValidatorContext(config, height, readOnlyCache);

			// - set up a custom mosaic id resolver
			const_cast<model::ResolverContext&>(context.Resolvers) = test::CreateResolverContextWithCustomDoublingMosaicResolver();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, context);

			// Assert:
			EXPECT_EQ(expectedResult, result)
							<< "notificationPropertyFlagMask " << static_cast<uint16_t>(notificationPropertyFlagMask)
							<< "mosaicPropertyFlagMask " << static_cast<uint16_t>(mosaicPropertyFlagMask);
		}
	}
#define MOSAIC_ID_TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Resolved_V2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ResolvedMosaicTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Unresolved_V2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<UnresolvedMosaicTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	MOSAIC_ID_TRAITS_BASED_TEST(FailureWhenPropertyFlagMaskDoesNotOverlapSet) {
		// Assert: 101, 010
		AssertPropertyFlagMaskValidationResult(Failure_Mosaic_Required_Property_Flag_Unset, TTraits::Default_Id, 0x05, 0x02, model::MosaicRequirementAction::Set);
	}

	MOSAIC_ID_TRAITS_BASED_TEST(FailureWhenPropertyFlagMaskPartiallyOverlapsSet) {
		// Assert: 101, 110
		AssertPropertyFlagMaskValidationResult(Failure_Mosaic_Required_Property_Flag_Unset, TTraits::Default_Id, 0x05, 0x06, model::MosaicRequirementAction::Set);
	}

	MOSAIC_ID_TRAITS_BASED_TEST(SuccessWhenPropertyFlagMaskIsExactMatchSet) {
		// Assert: 101, 101
		AssertPropertyFlagMaskValidationResult(ValidationResult::Success, TTraits::Default_Id, 0x05, 0x05, model::MosaicRequirementAction::Set);
	}

	MOSAIC_ID_TRAITS_BASED_TEST(SuccessWhenPropertyFlagMaskIsSubsetSet) {
		// Assert: 101, 111
		AssertPropertyFlagMaskValidationResult(ValidationResult::Success, TTraits::Default_Id, 0x05, 0x07, model::MosaicRequirementAction::Set);
	}

	MOSAIC_ID_TRAITS_BASED_TEST(FailureWhenPropertyFlagMaskDoesNotOverlapUnset) {
		// Assert: 101, 010
		AssertPropertyFlagMaskValidationResult(Failure_Mosaic_Required_Property_Flag_Unset, TTraits::Default_Id, 0x05, 0x05, model::MosaicRequirementAction::Unset);
	}

	MOSAIC_ID_TRAITS_BASED_TEST(FailureWhenPropertyFlagMaskPartiallyOverlapsUnset) {
		// Assert: 101, 110
		AssertPropertyFlagMaskValidationResult(Failure_Mosaic_Required_Property_Flag_Unset, TTraits::Default_Id, 0x05, 0x01, model::MosaicRequirementAction::Unset);
	}

	MOSAIC_ID_TRAITS_BASED_TEST(SuccessWhenPropertyFlagMaskIsExactMatchUnset) {
		// Assert: 101, 101
		AssertPropertyFlagMaskValidationResult(ValidationResult::Success, TTraits::Default_Id, 0x05, 0x02, model::MosaicRequirementAction::Unset);
	}

	MOSAIC_ID_TRAITS_BASED_TEST(SuccessWhenPropertyFlagMaskIsSubsetUnset) {
		// Assert: 101, 111
		AssertPropertyFlagMaskValidationResult(ValidationResult::Success, TTraits::Default_Id, 0x05, 0x00, model::MosaicRequirementAction::Unset);
	}
}}
