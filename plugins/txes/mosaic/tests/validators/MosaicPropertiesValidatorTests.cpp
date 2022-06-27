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
#include "catapult/constants.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS MosaicPropertiesValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(MosaicPropertiesV1)
	DEFINE_COMMON_VALIDATOR_TESTS(MosaicPropertiesV2)

	namespace {
		constexpr auto Max_Divisibility = std::numeric_limits<uint8_t>::max();
		constexpr BlockDuration Max_Duration(std::numeric_limits<BlockDuration::ValueType>::max());
	}

	// region flags

	namespace {

		struct V1TestTraits
		{
			using Notification = model::MosaicPropertiesNotification<1>;
			static constexpr auto ValidFlags = { 0x00, 0x02, 0x03 };
			static stateful::NotificationValidatorPointerT<Notification> Create(){
				return CreateMosaicPropertiesV1Validator();
			}
		};

		struct V2TestTraits
		{
			using Notification = model::MosaicPropertiesNotification<2>;
			static constexpr auto ValidFlags = { 0x00, 0x02, 0x03, 0x04, 0x08, 0x10, 0x1F};
			static stateful::NotificationValidatorPointerT<Notification> Create(){
				return CreateMosaicPropertiesV2Validator();
			}
		};

		auto CreateConfig(uint8_t maxMosaicDivisibility, const BlockDuration& maxMosaicDuration) {
			auto pluginConfig = config::MosaicConfiguration::Uninitialized();
			pluginConfig.MaxMosaicDivisibility = maxMosaicDivisibility;
			if (maxMosaicDuration == Max_Duration) {
				auto duration = maxMosaicDuration.unwrap() / utils::TimeSpan::FromHours(1).millis();
				pluginConfig.MaxMosaicDuration = utils::BlockSpan::FromHours(duration);
			} else {
				pluginConfig.MaxMosaicDuration = utils::BlockSpan::FromHours(maxMosaicDuration.unwrap());
			}
			test::MutableBlockchainConfiguration config;
			config.Network.BlockGenerationTargetTime = utils::TimeSpan::FromHours(1);
			config.Network.SetPluginConfiguration(pluginConfig);
			return config.ToConst();
		}
		auto Default_Config = CreateConfig(Max_Divisibility, Max_Duration);

		template<typename TTestTraits>
		void AssertFlagsResult(ValidationResult expectedResult, model::MosaicFlags flags) {
			// Arrange:
			auto cache = test::CreateEmptyCatapultCache(Default_Config);
			auto pValidator = TTestTraits::Create();
			model::MosaicPropertiesHeader header{};
			header.Flags = flags;
			auto notification = typename TTestTraits::Notification(header, nullptr);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, Default_Config);

			// Assert:
			EXPECT_EQ(expectedResult, result) << "flags " << static_cast<uint16_t>(flags);
		}
	}
#define TRAITS_BASED_TEST(TEST_CLASS, TEST_NAME) \
    template<typename TTestTraits>                                 \
	void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_v1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<V1TestTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_v2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<V2TestTraits>(); } \
    template<typename TTestTraits>                                 \
	void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()


	TRAITS_BASED_TEST(TEST_CLASS, SuccessWhenValidatingValidMosaicFlags) {
		// Assert:
		for (auto flags : TTestTraits::ValidFlags)
			AssertFlagsResult<TTestTraits>(ValidationResult::Success, static_cast<model::MosaicFlags>(flags));
	}

	TRAITS_BASED_TEST(TEST_CLASS, FailureWhenValidatingInvalidMosaicFlags) {
		// Assert:
		auto lastValidFlag = *(TTestTraits::ValidFlags.end()-1);
		for (auto flags : { lastValidFlag+0x01, lastValidFlag+0x02, lastValidFlag+0x03, lastValidFlag+0x04, 0xFF })
			AssertFlagsResult<TTestTraits>(Failure_Mosaic_Invalid_Flags, static_cast<model::MosaicFlags>(flags));
	}

	// endregion

	// region divisibility

	namespace {
		template<typename TTestTraits>
		void AssertDivisibilityValidationResult(ValidationResult expectedResult, uint8_t divisibility, uint8_t maxDivisibility) {
			// Arrange:
			auto config = CreateConfig(maxDivisibility, Max_Duration);
			auto cache = test::CreateEmptyCatapultCache(config);
			auto pValidator = TTestTraits::Create();
			model::MosaicPropertiesHeader header{};
			header.Divisibility = divisibility;
			auto notification = typename TTestTraits::Notification(header, nullptr);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, config);

			// Assert:
			EXPECT_EQ(expectedResult, result)
					<< "divisibility " << static_cast<uint32_t>(divisibility)
					<< ", max " << static_cast<uint32_t>(maxDivisibility);
		}
	}

	TRAITS_BASED_TEST(TEST_CLASS, SuccessWhenValidatingDivisibilityLessThanMax) {
		// Assert:
		AssertDivisibilityValidationResult<TTestTraits>(ValidationResult::Success, 7, 11);
	}

	TRAITS_BASED_TEST(TEST_CLASS, SuccessWhenValidatingDivisibilityEqualToMax) {
		// Assert:
		AssertDivisibilityValidationResult<TTestTraits>(ValidationResult::Success, 11, 11);
	}

	TRAITS_BASED_TEST(TEST_CLASS, FailureWhenValidatingDivisibilityGreaterThanMax) {
		// Assert:
		AssertDivisibilityValidationResult<TTestTraits>(Failure_Mosaic_Invalid_Divisibility, 12, 11);
		AssertDivisibilityValidationResult<TTestTraits>(Failure_Mosaic_Invalid_Divisibility, 111, 11);
	}

	// endregion

	// region duration

	namespace {
		template<typename TTestTraits>
		void AssertDurationValidationResult(ValidationResult expectedResult, uint16_t duration, uint16_t maxDuration) {
			// Arrange:
			auto config = CreateConfig(Max_Divisibility, BlockDuration(maxDuration));
			auto cache = test::CreateEmptyCatapultCache(config);
			auto pValidator = TTestTraits::Create();
			model::MosaicPropertiesHeader header{};
			header.Count = 1;
			auto properties = std::vector<model::MosaicProperty>{ { model::MosaicPropertyId::Duration, duration } };
			auto notification = typename TTestTraits::Notification(header, properties.data());

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, config);

			// Assert:
			EXPECT_EQ(expectedResult, result) << "duration " << duration << ", maxDuration " << maxDuration;
		}
	}

	TRAITS_BASED_TEST(TEST_CLASS, SuccessWhenValidatingDurationLessThanMax) {
		// Assert:
		AssertDurationValidationResult<TTestTraits>(ValidationResult::Success, 12312, 12345);
	}

	TRAITS_BASED_TEST(TEST_CLASS, SuccessWhenValidatingDurationEqualToMax) {
		// Assert:
		AssertDurationValidationResult<TTestTraits>(ValidationResult::Success, 12345, 12345);
	}

	TRAITS_BASED_TEST(TEST_CLASS, FailureWhenValidatingDurationGreaterThanMax) {
		// Assert:
		AssertDurationValidationResult<TTestTraits>(Failure_Mosaic_Invalid_Duration, 12346, 12345);
		AssertDurationValidationResult<TTestTraits>(Failure_Mosaic_Invalid_Duration, 65432, 12345);
	}

	TRAITS_BASED_TEST(TEST_CLASS, FailuresWhenValidatingZeroDuration) {
		// Assert: eternal duration is allowed but cannot be specified explicitly
		AssertDurationValidationResult<TTestTraits>(Failure_Mosaic_Invalid_Duration, 0, 12345);
	}

	// endregion

	// region optional properties

	TRAITS_BASED_TEST(TEST_CLASS, SuccessWhenValidatingMosaicWithNoOptionalProperties) {
		// Arrange:
		auto cache = test::CreateEmptyCatapultCache(Default_Config);
		auto pValidator = TTestTraits::Create();
		model::MosaicPropertiesHeader header{};
		header.Count = 0;
		auto notification = typename TTestTraits::Notification(header, nullptr);

		// Act:
		auto result = test::ValidateNotification(*pValidator, notification, cache, Default_Config);

		// Assert:
		EXPECT_EQ(ValidationResult::Success, result);
	}

	TRAITS_BASED_TEST(TEST_CLASS, SuccessWhenValidatingMosaicWithDurationOptionalProperty) {
		// Arrange:
		auto cache = test::CreateEmptyCatapultCache(Default_Config);;
		auto pValidator = TTestTraits::Create();
		model::MosaicPropertiesHeader header{};
		header.Count = 1;
		auto properties = std::vector<model::MosaicProperty>{ { model::MosaicPropertyId::Duration, 123 } };
		auto notification = typename TTestTraits::Notification(header, properties.data());

		// Act:
		auto result = test::ValidateNotification(*pValidator, notification, cache, Default_Config);

		// Assert:
		EXPECT_EQ(ValidationResult::Success, result);
	}

	namespace {
		template<typename TTestTraits>
		void AssertInvalidOptionalProperty(
				const model::MosaicProperty& property,
				ValidationResult expectedResult = Failure_Mosaic_Invalid_Property) {
			// Arrange: create a transaction with a single property
			auto cache = test::CreateEmptyCatapultCache(Default_Config);
			auto pValidator = TTestTraits::Create();
			model::MosaicPropertiesHeader header{};
			header.Count = 1;
			auto notification = typename TTestTraits::Notification(header, &property);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, Default_Config);

			// Assert:
			EXPECT_EQ(expectedResult, result)
					<< "property id " << static_cast<uint16_t>(property.Id)
					<< ", property value " << property.Value;
		}
	}

	TRAITS_BASED_TEST(TEST_CLASS, FailureWhenValidatingMosaicWithInvalidOptionalProperty) {
		// Assert:
		AssertInvalidOptionalProperty<TTestTraits>({ model::MosaicPropertyId::Divisibility, 123 });
	}

	TRAITS_BASED_TEST(TEST_CLASS, FailureWhenValidatingMosaicWithUnkownOptionalProperty) {
		// Assert:
		AssertInvalidOptionalProperty<TTestTraits>({ model::MosaicPropertyId::Sentinel_Property_Id, 123 });
	}

	TRAITS_BASED_TEST(TEST_CLASS, FailureWhenValidatingMosaicWithKnownOptionalPropertyWithDefaultValue) {
		// Assert:
		AssertInvalidOptionalProperty<TTestTraits>(
				{ model::MosaicPropertyId::Duration, Eternal_Artifact_Duration.unwrap() },
				Failure_Mosaic_Invalid_Duration);
	}

	namespace {
		template<typename TTestTraits>
		void AssertInvalidOptionalPropertyCount(uint8_t count) {
			// Arrange: indicate the transaction contains extra properties
			//          (validator will reject the transaction before dereferencing the extra properties)
			auto cache = test::CreateEmptyCatapultCache(Default_Config);
			auto pValidator = TTestTraits::Create();
			model::MosaicPropertiesHeader header{};
			header.Count = count;
			auto notification = typename TTestTraits::Notification(header, nullptr);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, Default_Config);

			// Assert:
			EXPECT_EQ(Failure_Mosaic_Invalid_Property, result);
		}
	}

	TRAITS_BASED_TEST(TEST_CLASS, FailureWhenValidatingMosaicWithTooManyOptionalProperties) {
		// Assert:
		AssertInvalidOptionalPropertyCount<TTestTraits>(2);
		AssertInvalidOptionalPropertyCount<TTestTraits>(100);
	}

	// endregion
}}
