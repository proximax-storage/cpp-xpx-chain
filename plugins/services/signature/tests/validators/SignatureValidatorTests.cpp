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
#include "catapult/crypto/Signer.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

	DEFINE_COMMON_VALIDATOR_TESTS(Signature, GenerationHash())

#define TEST_CLASS SignatureValidatorTests

	namespace {
		using ReplayProtectionMode = model::SignatureNotification<1>::ReplayProtectionMode;

		void AssertValidationResult(
				ValidationResult expectedResult,
				const GenerationHash& generationHash,
				const model::SignatureNotification<1>& notification) {
			// Arrange:
			auto pValidator = CreateSignatureValidator(generationHash);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}

		struct TestContext {
		public:
			explicit TestContext(ReplayProtectionMode mode)
					: SignerKeyPair(test::GenerateKeyPair())
					, GenerationHash(test::GenerateRandomByteArray<catapult::GenerationHash>())
					, DataBuffer(test::GenerateRandomVector(55)) {
				// when replay protection is enabled, data buffer should be prepended by generation hash
				if (ReplayProtectionMode::Enabled == mode)
					crypto::Sign(SignerKeyPair, { GenerationHash, DataBuffer }, Signature);
				else
					crypto::Sign(SignerKeyPair, DataBuffer, Signature);
			}

		public:
			crypto::KeyPair SignerKeyPair;
			catapult::GenerationHash GenerationHash;
			std::vector<uint8_t> DataBuffer;
			catapult::Signature Signature;
		};
	}

#define ALL_REPLAY_PROTECTION_MODES_TEST(TEST_NAME) \
	template<ReplayProtectionMode Mode> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_ReplayProtectionEnabled) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ReplayProtectionMode::Enabled>(); } \
	TEST(TEST_CLASS, TEST_NAME##_ReplayProtectionDisabled) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ReplayProtectionMode::Disabled>(); } \
	template<ReplayProtectionMode Mode> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	ALL_REPLAY_PROTECTION_MODES_TEST(SuccessWhenValidatingValidSignature) {
		// Arrange:
		TestContext context(Mode);
		model::SignatureNotification<1> notification(context.SignerKeyPair.publicKey(), context.Signature, context.DataBuffer, Mode);

		// Assert:
		AssertValidationResult(ValidationResult::Success, context.GenerationHash, notification);
	}

	ALL_REPLAY_PROTECTION_MODES_TEST(FailureWhenSignatureIsAltered) {
		// Arrange:
		TestContext context(Mode);
		model::SignatureNotification<1> notification(context.SignerKeyPair.publicKey(), context.Signature, context.DataBuffer, Mode);

		context.Signature[0] ^= 0xFF;

		// Assert:
		AssertValidationResult(Failure_Signature_Not_Verifiable, context.GenerationHash, notification);
	}

	ALL_REPLAY_PROTECTION_MODES_TEST(FailureWhenDataIsAltered) {
		// Arrange:
		TestContext context(Mode);
		model::SignatureNotification<1> notification(context.SignerKeyPair.publicKey(), context.Signature, context.DataBuffer, Mode);

		context.DataBuffer[10] ^= 0xFF;

		// Assert:
		AssertValidationResult(Failure_Signature_Not_Verifiable, context.GenerationHash, notification);
	}

	TEST(TEST_CLASS, FailureWhenGenerationHashIsAltered_ReplayProtectionEnabled) {
		// Arrange:
		auto mode = ReplayProtectionMode::Enabled;
		TestContext context(mode);
		model::SignatureNotification<1> notification(context.SignerKeyPair.publicKey(), context.Signature, context.DataBuffer, mode);

		context.GenerationHash[2] ^= 0xFF;

		// Assert:
		AssertValidationResult(Failure_Signature_Not_Verifiable, context.GenerationHash, notification);
	}

	TEST(TEST_CLASS, SuccessWhenGenerationHashIsAltered_ReplayProtectionDisabled) {
		// Arrange:
		auto mode = ReplayProtectionMode::Disabled;
		TestContext context(mode);
		model::SignatureNotification<1> notification(context.SignerKeyPair.publicKey(), context.Signature, context.DataBuffer, mode);

		context.GenerationHash[2] ^= 0xFF;

		// Assert:
		AssertValidationResult(ValidationResult::Success, context.GenerationHash, notification);
	}
}}
