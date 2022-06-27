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

#include "src/config/MosaicConfiguration.h"
#include "src/validators/Validators.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/MosaicCacheTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS MosaicSupplyChangeAllowedValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(MosaicSupplyChangeAllowedV1)
	DEFINE_COMMON_VALIDATOR_TESTS(MosaicSupplyChangeAllowedV2)


	namespace {
		struct V1TestTraits
		{
			using Notification = model::MosaicSupplyChangeNotification<1>;
			static stateful::NotificationValidatorPointerT<Notification> Create(){
				return CreateMosaicSupplyChangeAllowedV1Validator();
			}
		};

		struct V2TestTraits
		{
			using Notification = model::MosaicSupplyChangeNotification<2>;
			static stateful::NotificationValidatorPointerT<Notification> Create(){
				return CreateMosaicSupplyChangeAllowedV2Validator();
			}
		};
		constexpr auto Max_Atomic_Units = Amount(std::numeric_limits<Amount::ValueType>::max());

		template<typename TTestTraits>
		void AssertValidationResult(
				ValidationResult expectedResult,
				const cache::CatapultCache& cache,
				Height height,
				const typename TTestTraits::Notification& notification,
				Amount maxAtomicUnits = Max_Atomic_Units) {
			// Arrange:
			auto networkConfig = model::NetworkConfiguration::Uninitialized();
			networkConfig.MaxMosaicAtomicUnits = maxAtomicUnits;
			auto pConfigHolder = config::CreateMockConfigurationHolder(networkConfig);
			auto pValidator = TTestTraits::Create();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, pConfigHolder->Config(), height);

			// Assert:
			EXPECT_EQ(expectedResult, result) << "id " << notification.MosaicId << ", delta " << notification.Delta;
		}

		void AddMosaic(
				cache::CatapultCache& cache,
				MosaicId id,
				Amount mosaicSupply,
				const Key& owner,
				Amount ownerSupply,
				model::MosaicFlags flags = model::MosaicFlags::Supply_Mutable) {
			auto delta = cache.createDelta();

			// add a mosaic definition with the desired flags
			model::MosaicProperties::PropertyValuesContainer values{};
			values[utils::to_underlying_type(model::MosaicPropertyId::Flags)] = utils::to_underlying_type(flags);

			auto& mosaicCacheDelta = delta.sub<cache::MosaicCache>();
			auto definition = state::MosaicDefinition(Height(50), Key(), 3, model::MosaicProperties::FromValues(values));
			auto entry = state::MosaicEntry(id, definition);
			entry.increaseSupply(mosaicSupply);
			mosaicCacheDelta.insert(entry);

			test::AddMosaicOwner(delta, id, owner, ownerSupply);
			cache.commit(Height());
		}
	}

	// region immutable supply

	namespace {
		template<typename TTestTraits>
		void AssertCanChangeImmutableSupplyWhenOwnerHasCompleteSupply(model::MosaicSupplyChangeDirection direction) {
			// Arrange:
			auto signer = test::GenerateRandomByteArray<Key>();
			auto notification = typename TTestTraits::Notification(signer, test::UnresolveXor(MosaicId(123)), direction, Amount(100));

			auto cache = test::MosaicCacheFactory::Create();
			AddMosaic(cache, MosaicId(123), Amount(500), signer, Amount(500), model::MosaicFlags::None);

			// Assert:
			AssertValidationResult<TTestTraits>(ValidationResult::Success, cache, Height(100), notification);
		}

		void AssertCannotChangeImmutableSupplyWhenOwnerHasCompleteSupplyAndImmutableFlagIsSet(model::MosaicSupplyChangeDirection direction) {
			// Arrange:
			auto signer = test::GenerateRandomByteArray<Key>();
			auto notification = model::MosaicSupplyChangeNotification<2>(signer, test::UnresolveXor(MosaicId(123)), direction, Amount(100));

			auto cache = test::MosaicCacheFactory::Create();
			AddMosaic(cache, MosaicId(123), Amount(500), signer, Amount(500), model::MosaicFlags::Supply_Force_Immutable);

			// Assert:
			AssertValidationResult<V2TestTraits>(Failure_Mosaic_Supply_Immutable, cache, Height(100), notification);
		}
	}

#define TRAITS_BASED_TEST(TEST_CLASS, TEST_NAME) \
    template<typename TTestTraits>                                 \
	void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_v1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<V1TestTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_v2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<V2TestTraits>(); } \
    template<typename TTestTraits>                                 \
	void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	TRAITS_BASED_TEST(TEST_CLASS, CanIncreaseImmutableSupplyWhenOwnerHasCompleteSupply) {
		// Assert:
		AssertCanChangeImmutableSupplyWhenOwnerHasCompleteSupply<TTestTraits>(model::MosaicSupplyChangeDirection::Increase);
	}

	TRAITS_BASED_TEST(TEST_CLASS, CanDecreaseImmutableSupplyWhenOwnerHasCompleteSupply) {
		// Assert:
		AssertCanChangeImmutableSupplyWhenOwnerHasCompleteSupply<TTestTraits>(model::MosaicSupplyChangeDirection::Decrease);
	}

	TEST(TEST_CLASS, CannotIncreaseImmutableSupplyWithImmutableFlagSetWhenownerHasCompleteSupply) {
		// Assert:
		AssertCannotChangeImmutableSupplyWhenOwnerHasCompleteSupplyAndImmutableFlagIsSet(model::MosaicSupplyChangeDirection::Increase);
	}

	TEST(TEST_CLASS, CannotDecreaseImmutableSupplyWithImmutableFlagSetWhenownerHasCompleteSupply) {
		// Assert:
		AssertCannotChangeImmutableSupplyWhenOwnerHasCompleteSupplyAndImmutableFlagIsSet(model::MosaicSupplyChangeDirection::Decrease);
	}

	namespace {
		template<typename TTestTraits>
		void AssertCannotChangeImmutableSupplyWhenOwnerHasPartialSupply(model::MosaicSupplyChangeDirection direction) {
			// Arrange:
			auto signer = test::GenerateRandomByteArray<Key>();
			auto notification = typename TTestTraits::Notification(signer, test::UnresolveXor(MosaicId(123)), direction, Amount(100));

			auto cache = test::MosaicCacheFactory::Create();
			AddMosaic(cache, MosaicId(123), Amount(500), signer, Amount(499), model::MosaicFlags::None);

			// Assert:
			AssertValidationResult<TTestTraits>(Failure_Mosaic_Supply_Immutable, cache, Height(100), notification);
		}
	}

	TRAITS_BASED_TEST(TEST_CLASS, CannotIncreaseImmutableSupplyWhenOwnerHasPartialSupply) {
		// Assert:
		AssertCannotChangeImmutableSupplyWhenOwnerHasPartialSupply<TTestTraits>(model::MosaicSupplyChangeDirection::Increase);
	}

	TRAITS_BASED_TEST(TEST_CLASS, CannotDecreaseImmutableSupplyWhenOwnerHasPartialSupply) {
		// Assert:
		AssertCannotChangeImmutableSupplyWhenOwnerHasPartialSupply<TTestTraits>(model::MosaicSupplyChangeDirection::Decrease);
	}

	// endregion

	// region decrease supply

	namespace {
		template<typename TTestTraits>
		void AssertDecreaseValidationResult(ValidationResult expectedResult, Amount mosaicSupply, Amount ownerSupply, Amount delta) {
			// Arrange:
			auto signer = test::GenerateRandomByteArray<Key>();
			auto direction = model::MosaicSupplyChangeDirection::Decrease;
			auto notification = typename TTestTraits::Notification(signer, test::UnresolveXor(MosaicId(123)), direction, delta);

			auto cache = test::MosaicCacheFactory::Create();
			AddMosaic(cache, MosaicId(123), mosaicSupply, signer, ownerSupply);

			// Assert:
			AssertValidationResult<TTestTraits>(expectedResult, cache, Height(100), notification);
		}
	}

	TRAITS_BASED_TEST(TEST_CLASS, CanDecreaseMutableSupplyByLessThanOwnerSupply) {
		// Assert:
		AssertDecreaseValidationResult<TTestTraits>(ValidationResult::Success, Amount(500), Amount(400), Amount(300));
		AssertDecreaseValidationResult<TTestTraits>(ValidationResult::Success, Amount(500), Amount(400), Amount(399));
	}

	TRAITS_BASED_TEST(TEST_CLASS, CanDecreaseMutableSupplyByEntireOwnerSupply) {
		// Assert:
		AssertDecreaseValidationResult<TTestTraits>(ValidationResult::Success, Amount(500), Amount(400), Amount(400));
	}

	TRAITS_BASED_TEST(TEST_CLASS, CannotDecreaseMutableSupplyByGreaterThanOwnerSupply) {
		// Assert:
		AssertDecreaseValidationResult<TTestTraits>(Failure_Mosaic_Supply_Negative, Amount(500), Amount(400), Amount(401));
		AssertDecreaseValidationResult<TTestTraits>(Failure_Mosaic_Supply_Negative, Amount(500), Amount(400), Amount(500));
	}

	// endregion

	// region increase

	namespace {
		template<typename TTestTraits>
		void AssertIncreaseValidationResult(ValidationResult expectedResult, Amount maxAtomicUnits, Amount mosaicSupply, Amount delta) {
			// Arrange:
			auto signer = test::GenerateRandomByteArray<Key>();
			auto direction = model::MosaicSupplyChangeDirection::Increase;
			auto notification = typename TTestTraits::Notification(signer, test::UnresolveXor(MosaicId(123)), direction, delta);

			auto cache = test::MosaicCacheFactory::Create();
			AddMosaic(cache, MosaicId(123), mosaicSupply, signer, Amount(111));

			// Assert:
			AssertValidationResult<TTestTraits>(expectedResult, cache, Height(100), notification, maxAtomicUnits);
		}
	}

	TRAITS_BASED_TEST(TEST_CLASS, CanIncreaseMutableSupplyToLessThanAtomicUnits) {
		// Assert:
		AssertIncreaseValidationResult<TTestTraits>(ValidationResult::Success, Amount(900), Amount(500), Amount(300));
		AssertIncreaseValidationResult<TTestTraits>(ValidationResult::Success, Amount(900), Amount(500), Amount(399));
	}

	TRAITS_BASED_TEST(TEST_CLASS, CanIncreaseMutableSupplyToExactlyAtomicUnits) {
		// Assert:
		AssertIncreaseValidationResult<TTestTraits>(ValidationResult::Success, Amount(900), Amount(500), Amount(400));
	}

	TRAITS_BASED_TEST(TEST_CLASS, CannotIncreaseMutableSupplyToGreaterThanAtomicUnits) {
		// Assert:
		AssertIncreaseValidationResult<TTestTraits>(Failure_Mosaic_Supply_Exceeded, Amount(900), Amount(500), Amount(401));
		AssertIncreaseValidationResult<TTestTraits>(Failure_Mosaic_Supply_Exceeded, Amount(900), Amount(500), Amount(500));
	}

	TRAITS_BASED_TEST(TEST_CLASS, CannotIncreaseMutableSupplyWhenOverflowIsDetected) {
		// Assert:
		AssertIncreaseValidationResult<TTestTraits>(Failure_Mosaic_Supply_Exceeded, Amount(900), Amount(500), Max_Atomic_Units);
	}
	
	// endregion
}}
