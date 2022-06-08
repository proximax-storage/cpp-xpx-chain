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

#define TEST_CLASS StrictAggregateCosignaturesValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(StrictAggregateCosignaturesV1)
	DEFINE_COMMON_VALIDATOR_TESTS(StrictAggregateCosignaturesV3)

	namespace {
		using Keys = std::vector<Key>;

		struct V1ValidatorFactory
		{
			using Notification = model::AggregateCosignaturesNotification<1>;
			using Descriptor = model::AggregateTransactionRawDescriptor;
			static stateful::NotificationValidatorPointerT<Notification> Create(){
				return CreateStrictAggregateCosignaturesV1Validator();
			}
		};
		struct V3ValidatorFactory
		{
			using Notification = model::AggregateCosignaturesNotification<3>;
			using Descriptor = model::AggregateTransactionExtendedDescriptor;
			static stateful::NotificationValidatorPointerT<Notification> Create(){
				return CreateStrictAggregateCosignaturesV3Validator();
			}
		};

		template<typename TValidatorFactory>
		void AssertValidationResult(ValidationResult expectedResult, const Key& signer, const Keys& cosigners, const Keys& txSigners) {
			// Arrange:
			// - setup transactions
			std::vector<uint8_t> txBuffer(sizeof(model::EmbeddedTransaction) * txSigners.size());
			auto* pTransactions = reinterpret_cast<model::EmbeddedTransaction*>(txBuffer.data());
			for (auto i = 0u; i < txSigners.size(); ++i) {
				pTransactions[i].Signer = txSigners[i];
				pTransactions[i].Size = sizeof(model::EmbeddedTransaction);
			}

			// - setup cosignatures
			auto cosignatures = test::GenerateRandomDataVector<typename TValidatorFactory::Descriptor::CosignatureType>(cosigners.size());
			for (auto i = 0u; i < cosigners.size(); ++i)
				cosignatures[i].Signer = cosigners[i];

			typename TValidatorFactory::Notification notification(signer, txSigners.size(), pTransactions, cosigners.size(), cosignatures.data());

			auto pluginConfig = config::AggregateConfiguration::Uninitialized();
			pluginConfig.EnableStrictCosignatureCheck = true;
			test::MutableBlockchainConfiguration mutableConfig;
			mutableConfig.Network.SetPluginConfiguration(pluginConfig);
			auto config = mutableConfig.ToConst();
			auto cache = test::CreateEmptyCatapultCache(config);
			auto pValidator = TValidatorFactory::Create();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, config);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}
#define TRAITS_BASED_TEST(TEST_NAME) \
    template<typename TValidatorFactory>                                 \
	void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(VersionType version); \
	TEST(TEST_CLASS, TEST_NAME##_v2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<V1ValidatorFactory>(2); } \
	TEST(TEST_CLASS, TEST_NAME##_v3) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<V3ValidatorFactory>(3); } \
    template<typename TValidatorFactory>                                 \
	void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(VersionType version)

	// region success

	TRAITS_BASED_TEST(SuccessWhenTransactionSignersExactlyMatchCosigners) {
		// Arrange:
		auto signer = test::GenerateRandomByteArray<Key>();
		auto cosigners = test::GenerateRandomDataVector<Key>(3);
		auto txSigners = Keys{ signer, cosigners[0], cosigners[1], cosigners[2] };

		// Assert:
		AssertValidationResult<TValidatorFactory>(ValidationResult::Success, signer, cosigners, txSigners);
	}

	TRAITS_BASED_TEST(SuccessWhenTransactionSignersExactlyMatchCosignersOutOfOrder) {
		// Arrange:
		auto signer = test::GenerateRandomByteArray<Key>();
		auto cosigners = test::GenerateRandomDataVector<Key>(3);
		auto txSigners = Keys{ cosigners[2], cosigners[0], signer, cosigners[1] };

		// Assert:
		AssertValidationResult<TValidatorFactory>(ValidationResult::Success, signer, cosigners, txSigners);
	}

	TRAITS_BASED_TEST(SuccessWhenAllTransactionsHaveSameSignerAsAggregate) {
		// Arrange:
		auto signer = test::GenerateRandomByteArray<Key>();
		auto txSigners = Keys{ signer, signer, signer };

		// Assert:
		AssertValidationResult<TValidatorFactory>(ValidationResult::Success, signer, {}, txSigners);
	}

	// endregion

	// region failure

	TRAITS_BASED_TEST(FailureWhenTransactionSignerIsNotMachedByCosigner) {
		// Arrange: there is an extra tx signer with no match
		auto signer = test::GenerateRandomByteArray<Key>();
		auto cosigners = test::GenerateRandomDataVector<Key>(3);
		auto txSigners = Keys{ signer, cosigners[0], cosigners[1], cosigners[2], test::GenerateRandomByteArray<Key>() };

		// Assert:
		AssertValidationResult<TValidatorFactory>(Failure_Aggregate_Missing_Cosigners, signer, cosigners, txSigners);
	}

	TRAITS_BASED_TEST(FailureWhenCosignerIsNotTransactionSigner) {
		// Arrange: there is a cosigner that doesn't match any tx
		auto signer = test::GenerateRandomByteArray<Key>();
		auto cosigners = test::GenerateRandomDataVector<Key>(3);
		auto txSigners = Keys{ signer, cosigners[0], cosigners[2] };

		// Assert:
		AssertValidationResult<TValidatorFactory>(Failure_Aggregate_Ineligible_Cosigners, signer, cosigners, txSigners);
	}

	TRAITS_BASED_TEST(FailureIneligibleDominatesFailureMissing) {
		// Arrange: there is an extra tx signer with no match and there is a cosigner that doesn't match any tx
		auto signer = test::GenerateRandomByteArray<Key>();
		auto cosigners = test::GenerateRandomDataVector<Key>(3);
		auto txSigners = Keys{ signer, cosigners[0], cosigners[2], test::GenerateRandomByteArray<Key>() };

		// Assert:
		AssertValidationResult<TValidatorFactory>(Failure_Aggregate_Ineligible_Cosigners, signer, cosigners, txSigners);
	}

	// endregion
#undef TRAITS_BASED_TEST
}}
