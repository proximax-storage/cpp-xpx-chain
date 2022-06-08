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

#include "src/model/AggregateTransaction.h"
#include "src/config/AggregateConfiguration.h"
#include "src/validators/Validators.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS BasicAggregateCosignaturesValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(BasicAggregateCosignaturesV1)
	DEFINE_COMMON_VALIDATOR_TESTS(BasicAggregateCosignaturesV3)

	namespace {
		template<typename TCosignatureType>
		auto GenerateRandomCosignatures(uint8_t numCosignatures) {
			return test::GenerateRandomDataVector<TCosignatureType>(numCosignatures);
		}

		auto CreateConfig(uint32_t maxTransactions, uint8_t maxCosignatures) {
			auto pluginConfig = config::AggregateConfiguration::Uninitialized();
			pluginConfig.MaxTransactionsPerAggregate = maxTransactions;
			pluginConfig.MaxCosignaturesPerAggregate = maxCosignatures;
			test::MutableBlockchainConfiguration config;
			config.Network.SetPluginConfiguration(pluginConfig);
			return config.ToConst();
		}
	}

	// region transactions count

	namespace {

		struct V1ValidatorFactory
		{
			using Notification = model::AggregateCosignaturesNotification<1>;
			using Descriptor = model::AggregateTransactionRawDescriptor;
			static stateful::NotificationValidatorPointerT<Notification> Create(){
				return CreateBasicAggregateCosignaturesV1Validator();
			}
		};
		struct V3ValidatorFactory
		{
			using Notification = model::AggregateCosignaturesNotification<3>;
			using Descriptor = model::AggregateTransactionExtendedDescriptor;
			static stateful::NotificationValidatorPointerT<Notification> Create(){
				return CreateBasicAggregateCosignaturesV3Validator();
			}
		};
		template<typename TValidatorFactory>
		void AssertMaxTransactionsValidationResult(ValidationResult expectedResult, uint32_t numTransactions, uint32_t maxTransactions) {
			// Arrange: notice that transaction data is not actually checked
			auto signer = test::GenerateRandomByteArray<Key>();
			typename TValidatorFactory::Notification notification(signer, numTransactions, nullptr, 0, static_cast<model::Cosignature<SignatureLayout::Raw> *>(nullptr));
			auto config = CreateConfig(maxTransactions, std::numeric_limits<uint8_t>::max());
			auto cache = test::CreateEmptyCatapultCache(config);
			auto pValidator = TValidatorFactory::Create();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, config);

			// Assert:
			EXPECT_EQ(expectedResult, result) << "txes " << numTransactions << ", max " << maxTransactions;
		}
	}
#define TRAITS_BASED_TEST(TEST_NAME) \
    template<typename TValidatorFactory>                                 \
	void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(VersionType version); \
	TEST(TEST_CLASS, TEST_NAME##_v2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<V1ValidatorFactory>(2); } \
	TEST(TEST_CLASS, TEST_NAME##_v3) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<V3ValidatorFactory>(3); } \
    template<typename TValidatorFactory>                                 \
	void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(VersionType version)

	TRAITS_BASED_TEST(FailureWhenValidatingNotificationWithZeroTransactions) {
		// Assert:
		AssertMaxTransactionsValidationResult<TValidatorFactory>(Failure_Aggregate_No_Transactions, 0, 100);
	}

	TRAITS_BASED_TEST(SuccessWhenValidatingNotificationWithLessThanMaxTransactions) {
		// Assert:
		AssertMaxTransactionsValidationResult<TValidatorFactory>(ValidationResult::Success, 1, 100);
		AssertMaxTransactionsValidationResult<TValidatorFactory>(ValidationResult::Success, 99, 100);
	}

	TRAITS_BASED_TEST(SuccessWhenValidatingNotificationWithExactlyMaxTransactions) {
		// Assert:
		AssertMaxTransactionsValidationResult<TValidatorFactory>(ValidationResult::Success, 100, 100);
	}

	TRAITS_BASED_TEST(FailureWhenValidatingNotificationWithGreaterThanMaxTransactions) {
		// Assert:
		AssertMaxTransactionsValidationResult<TValidatorFactory>(Failure_Aggregate_Too_Many_Transactions, 101, 100);
		AssertMaxTransactionsValidationResult<TValidatorFactory>(Failure_Aggregate_Too_Many_Transactions, 999, 100);
	}

	// endregion

	// region cosignatures count

	namespace {
		template<typename TValidatorFactory>
		void AssertMaxCosignaturesValidationResult(ValidationResult expectedResult, uint8_t numCosignatures, uint8_t maxCosignatures) {
			// Arrange:
			auto signer = test::GenerateRandomByteArray<Key>();
			auto cosignatures = GenerateRandomCosignatures<typename TValidatorFactory::Descriptor::CosignatureType>(numCosignatures);
			typename TValidatorFactory::Notification notification(signer, 3, nullptr, numCosignatures, cosignatures.data());
			auto config = CreateConfig(std::numeric_limits<uint32_t>::max(), maxCosignatures);
			auto cache = test::CreateEmptyCatapultCache(config);
			auto pValidator = TValidatorFactory::Create();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, config);

			// Assert:
			EXPECT_EQ(expectedResult, result)
					<< "cosignatures " << static_cast<uint16_t>(numCosignatures)
					<< ", max " << static_cast<uint16_t>(maxCosignatures);
		}
	}

	TRAITS_BASED_TEST(SuccessWhenValidatingNotificationWithZeroExplicitCosignatures) {
		// Assert: notice that there is always one implicit cosigner (the tx signer)
		AssertMaxCosignaturesValidationResult<TValidatorFactory>(ValidationResult::Success, 0, 100);
	}

	TRAITS_BASED_TEST(SuccessWhenValidatingNotificationWithLessThanMaxCosignatures) {
		// Assert:
		AssertMaxCosignaturesValidationResult<TValidatorFactory>(ValidationResult::Success, 1, 100);
		AssertMaxCosignaturesValidationResult<TValidatorFactory>(ValidationResult::Success, 98, 100);
	}

	TRAITS_BASED_TEST(SuccessWhenValidatingNotificationWithExactlyMaxCosignatures) {
		// Assert:
		AssertMaxCosignaturesValidationResult<TValidatorFactory>(ValidationResult::Success, 99, 100);
	}

	TRAITS_BASED_TEST(FailureWhenValidatingNotificationWithGreaterThanMaxCosignatures) {
		// Assert:
		AssertMaxCosignaturesValidationResult<TValidatorFactory>(Failure_Aggregate_Too_Many_Cosignatures, 100, 100);
		AssertMaxCosignaturesValidationResult<TValidatorFactory>(Failure_Aggregate_Too_Many_Cosignatures, 222, 100);
	}

	// endregion

	// region cosigner uniqueness

	namespace {
		template<typename TValidatorFactory>
		void AssertCosignerUniquenessValidationResult(
				ValidationResult expectedResult,
				const Key& signer,
				const std::vector<typename TValidatorFactory::Descriptor::CosignatureType>& cosignatures) {
			// Arrange:
			typename TValidatorFactory::Notification notification(signer, 3, nullptr, cosignatures.size(), cosignatures.data());
			auto config = CreateConfig(
				std::numeric_limits<uint32_t>::max(),
				std::numeric_limits<uint8_t>::max());
			auto cache = test::CreateEmptyCatapultCache(config);
			auto pValidator = TValidatorFactory::Create();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, config);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TRAITS_BASED_TEST(SuccessWhenValidatingNotificationWithAllCosignersBeingUnique) {
		// Arrange: no conflicts
		auto signer = test::GenerateRandomByteArray<Key>();
		auto cosignatures = GenerateRandomCosignatures<typename TValidatorFactory::Descriptor::CosignatureType>(5);

		// Assert:
		AssertCosignerUniquenessValidationResult<TValidatorFactory>(ValidationResult::Success, signer, cosignatures);
	}

	TRAITS_BASED_TEST(FailureWhenValidatingNotificationWithRedundantExplicitAndImplicitCosigner) {
		// Arrange:
		auto signer = test::GenerateRandomByteArray<Key>();
		auto cosignatures = GenerateRandomCosignatures<typename TValidatorFactory::Descriptor::CosignatureType>(5);
		cosignatures[2].Signer = signer;

		// Assert:
		AssertCosignerUniquenessValidationResult<TValidatorFactory>(Failure_Aggregate_Redundant_Cosignatures, signer, cosignatures);
	}

	TRAITS_BASED_TEST(FailureWhenValidatingNotificationWithRedundantImplicitCosigners) {
		// Arrange:
		auto signer = test::GenerateRandomByteArray<Key>();
		auto cosignatures = GenerateRandomCosignatures<typename TValidatorFactory::Descriptor::CosignatureType>(5);
		cosignatures[0].Signer = cosignatures[4].Signer;

		// Assert:
		AssertCosignerUniquenessValidationResult<TValidatorFactory>(Failure_Aggregate_Redundant_Cosignatures, signer, cosignatures);
	}
#undef TRAITS_BASED_TEST
	// endregion
}}
