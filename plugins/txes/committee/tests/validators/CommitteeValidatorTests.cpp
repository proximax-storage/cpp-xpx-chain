/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/CommitteeTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS CommitteeValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(Committee, nullptr)

	namespace {
		using Notification = model::BlockCosignaturesNotification<1>;

		void AssertValidationResult(
				ValidationResult expectedResult,
				const Key& blockSigner,
				const std::vector<model::Cosignature> cosignatures,
				const chain::Committee& committee) {
			// Arrange:
			auto pAccountCollector = std::make_shared<cache::CommitteeAccountCollector>();
			auto pCommitteeManager = std::make_shared<test::TestWeightedVotingCommitteeManager>(pAccountCollector);
			pCommitteeManager->setCommittee(committee);
			auto pValidator = CreateCommitteeValidator(pCommitteeManager);
			Notification notification(blockSigner, cosignatures.size(), cosignatures.data(), 0, 0);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenBlockSignerInvalid) {
		// Arrange:
		chain::Committee committee;
		committee.BlockProposer = test::GenerateRandomByteArray<Key>();
		std::vector<model::Cosignature> cosignatures;

		// Assert:
		AssertValidationResult(
			Failure_Committee_Invalid_Block_Signer,
			test::GenerateRandomByteArray<Key>(),
			cosignatures,
			committee);
	}

	TEST(TEST_CLASS, FailureWhenCommitteeNumberInvalid) {
		// Arrange:
		chain::Committee committee;
		committee.BlockProposer = test::GenerateRandomByteArray<Key>();
		std::vector<model::Cosignature> cosignatures{
			{ test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Signature>() },
			{ test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Signature>() },
			{ test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Signature>() },
		};
		committee.Cosigners = {
			test::GenerateRandomByteArray<Key>(),
			test::GenerateRandomByteArray<Key>(),
		};

		// Assert:
		AssertValidationResult(
			Failure_Committee_Invalid_Committee_Number,
			committee.BlockProposer,
			cosignatures,
			committee);
	}

	TEST(TEST_CLASS, FailureWhenCommitteeInvalid) {
		// Arrange:
		chain::Committee committee;
		committee.BlockProposer = test::GenerateRandomByteArray<Key>();
		std::vector<model::Cosignature> cosignatures{
			{ test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Signature>() },
			{ test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Signature>() },
		};
		committee.Cosigners = {
			cosignatures[0].Signer,
			test::GenerateRandomByteArray<Key>(),
			test::GenerateRandomByteArray<Key>(),
		};

		// Assert:
		AssertValidationResult(
			Failure_Committee_Invalid_Committee,
			committee.BlockProposer,
			cosignatures,
			committee);
	}

	TEST(TEST_CLASS, Success) {
		// Arrange:
		chain::Committee committee;
		committee.BlockProposer = test::GenerateRandomByteArray<Key>();
		std::vector<model::Cosignature> cosignatures{
			{ test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Signature>() },
			{ test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Signature>() },
		};
		committee.Cosigners = {
			cosignatures[0].Signer,
			cosignatures[1].Signer,
			test::GenerateRandomByteArray<Key>(),
			test::GenerateRandomByteArray<Key>(),
		};

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			committee.BlockProposer,
			cosignatures,
			committee);
	}
}}
