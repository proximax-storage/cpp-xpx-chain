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

#include <tests/test/other/MutableBlockchainConfiguration.h>
#include "src/validators/Validators.h"
#include "catapult/crypto/Signer.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

	DEFINE_COMMON_VALIDATOR_TESTS(SignatureV1, GenerationHash())
	DEFINE_COMMON_VALIDATOR_TESTS(SignatureV2, GenerationHash())


#define TEST_CLASS SignatureValidatorTests

	namespace {
		const auto Max_Transaction_Lifetime = []() { return utils::TimeSpan::FromHours(2); }();
		constexpr auto Block_Time = Timestamp(8888);
		using ReplayProtectionMode = model::SignatureNotification<1>::ReplayProtectionMode;

		template<SignatureVersion TSignatureVersion>
		void AssertValidationResult(
				ValidationResult expectedResult,
				const GenerationHash& generationHash,
				const model::SignatureNotification<TSignatureVersion>& notification);

		template<>
		void AssertValidationResult<1>(
				ValidationResult expectedResult,
				const GenerationHash& generationHash,
				const model::SignatureNotification<1>& notification) {
			// Arrange:
			auto pValidator = CreateSignatureV1Validator(generationHash);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}

		template<>
		void AssertValidationResult<2>(
				ValidationResult expectedResult,
				const GenerationHash& generationHash,
				const model::SignatureNotification<2>& notification) {
			// Arrange:
			auto pValidator = CreateSignatureV2Validator(generationHash);

			test::MutableBlockchainConfiguration mutableConfig;
			mutableConfig.Network.MaxTransactionLifetime = Max_Transaction_Lifetime;
			mutableConfig.Network.EnableDeadlineValidation = true;
			auto config = mutableConfig.ToConst();
			auto cache = test::CreateEmptyCatapultCache(config);
			auto cacheView = cache.createView();
			auto readOnlyCache = cacheView.toReadOnly();
			auto resolverContext = test::CreateResolverContextXor();
			auto context = ValidatorContext(config, Height(123), Block_Time, resolverContext, readOnlyCache);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, context);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}

		template<uint32_t TSignerAccountVersion>
		struct TestContext {
		public:
			explicit TestContext(ReplayProtectionMode mode)
					: SignerKeyPair(test::GenerateKeyPair(TSignerAccountVersion))
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
			catapult::RawSignature Signature;
		};
	}


#define ALL_REPLAY_PROTECTION_MODES_TEST(TEST_NAME) \
	template<ReplayProtectionMode Mode> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_ReplayProtectionEnabled) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ReplayProtectionMode::Enabled>(); } \
	TEST(TEST_CLASS, TEST_NAME##_ReplayProtectionDisabled) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ReplayProtectionMode::Disabled>(); } \
	template<ReplayProtectionMode Mode> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	ALL_REPLAY_PROTECTION_MODES_TEST(SuccessWhenValidatingValidSignature) {

		// V1 account with deprecated V1 signature notification
		{
			// Arrange:
			TestContext<1> context(Mode);
			model::SignatureNotification<1> notification(context.SignerKeyPair.publicKey(), context.Signature, context.DataBuffer, Mode);

			// Assert:
			AssertValidationResult<1>(ValidationResult::Success, context.GenerationHash, notification);
		}

		// V1 account with V2 signature notification
		{
			// Arrange:
			TestContext<1> context(Mode);
			model::SignatureNotification<2> notification(context.SignerKeyPair.publicKey(), context.Signature, 1, context.DataBuffer, Mode);

			// Assert:
			AssertValidationResult<2>(ValidationResult::Success, context.GenerationHash, notification);
		}

		// V2 account with V2 signature notification
		{
			// Arrange:
			TestContext<2> context(Mode);
			model::SignatureNotification<2> notification(context.SignerKeyPair.publicKey(), context.Signature, 2, context.DataBuffer, Mode);

			// Assert:
			AssertValidationResult<2>(ValidationResult::Success, context.GenerationHash, notification);
		}
	}

	ALL_REPLAY_PROTECTION_MODES_TEST(FailureWhenSignatureIsAltered) {
		//V1 account with deprecated V1 signature notification
		{
			// Arrange:
			TestContext<1> context(Mode);
			model::SignatureNotification<1> notification(context.SignerKeyPair.publicKey(), context.Signature, context.DataBuffer, Mode);

			context.Signature[0] ^= 0xFF;

			// Assert:
			AssertValidationResult<1>(Failure_Signature_Not_Verifiable, context.GenerationHash, notification);
		}

		//V1 account with V2 signature notification
		{
			// Arrange:
			TestContext<1> context(Mode);
			model::SignatureNotification<2> notification(context.SignerKeyPair.publicKey(), context.Signature, 1, context.DataBuffer, Mode);

			context.Signature[0] ^= 0xFF;

			// Assert:
			AssertValidationResult<2>(Failure_Signature_Not_Verifiable, context.GenerationHash, notification);
		}

		//V2 account with V2 signature notification
		{
			// Arrange:
			TestContext<1> context(Mode);
			model::SignatureNotification<2> notification(context.SignerKeyPair.publicKey(), context.Signature, 2, context.DataBuffer, Mode);

			context.Signature[0] ^= 0xFF;

			// Assert:
			AssertValidationResult<2>(Failure_Signature_Not_Verifiable, context.GenerationHash, notification);
		}
	}

	ALL_REPLAY_PROTECTION_MODES_TEST(FailureWhenDataIsAltered) {


		//V1 account with deprecated V1 signature notification
		{
			// Arrange:
			TestContext<1> context(Mode);
			model::SignatureNotification<1> notification(context.SignerKeyPair.publicKey(), context.Signature, context.DataBuffer, Mode);

			context.DataBuffer[10] ^= 0xFF;

			// Assert:
			AssertValidationResult<1>(Failure_Signature_Not_Verifiable, context.GenerationHash, notification);
		}

		//V1 account with V2 signature notification
		{
			// Arrange:
			TestContext<1> context(Mode);
			model::SignatureNotification<2> notification(context.SignerKeyPair.publicKey(), context.Signature, 1, context.DataBuffer, Mode);

			context.DataBuffer[10] ^= 0xFF;

			// Assert:
			AssertValidationResult<2>(Failure_Signature_Not_Verifiable, context.GenerationHash, notification);
		}

		//V2 account with V2 signature notification
		{
			// Arrange:
			TestContext<1> context(Mode);
			model::SignatureNotification<2> notification(context.SignerKeyPair.publicKey(), context.Signature, 2, context.DataBuffer, Mode);

			context.DataBuffer[10] ^= 0xFF;

			// Assert:
			AssertValidationResult<2>(Failure_Signature_Not_Verifiable, context.GenerationHash, notification);
		}
	}

	TEST(TEST_CLASS, FailureWhenGenerationHashIsAltered_ReplayProtectionEnabled) {

		auto mode = ReplayProtectionMode::Enabled;

		//V1 account with deprecated V1 signature notification
		{
			// Arrange:
			TestContext<1> context(mode);
			model::SignatureNotification<1> notification(context.SignerKeyPair.publicKey(), context.Signature, context.DataBuffer, mode);

			context.GenerationHash[2] ^= 0xFF;

			// Assert:
			AssertValidationResult<1>(Failure_Signature_Not_Verifiable, context.GenerationHash, notification);
		}

		//V1 account with V2 signature notification
		{
			// Arrange:
			TestContext<1> context(mode);
			model::SignatureNotification<2> notification(context.SignerKeyPair.publicKey(), context.Signature, 1, context.DataBuffer, mode);

			context.GenerationHash[2] ^= 0xFF;

			// Assert:
			AssertValidationResult<2>(Failure_Signature_Not_Verifiable, context.GenerationHash, notification);
		}

		//V2 account with V2 signature notification
		{
			// Arrange:
			TestContext<2> context(mode);
			model::SignatureNotification<2> notification(context.SignerKeyPair.publicKey(), context.Signature, 2, context.DataBuffer, mode);

			context.GenerationHash[2] ^= 0xFF;

			// Assert:
			AssertValidationResult<2>(Failure_Signature_Not_Verifiable, context.GenerationHash, notification);
		}
	}

	TEST(TEST_CLASS, SuccessWhenGenerationHashIsAltered_ReplayProtectionDisabled) {

		auto mode = ReplayProtectionMode::Disabled;

		//V1 account with deprecated V1 signature notification
		{
			// Arrange:
			TestContext<1> context(mode);
			model::SignatureNotification<1> notification(context.SignerKeyPair.publicKey(), context.Signature, context.DataBuffer, mode);

			context.GenerationHash[2] ^= 0xFF;

			// Assert:
			AssertValidationResult<1>(ValidationResult::Success, context.GenerationHash, notification);
		}

		//V1 account with V2 signature notification
		{
			// Arrange:
			TestContext<1> context(mode);
			model::SignatureNotification<2> notification(context.SignerKeyPair.publicKey(), context.Signature, 1, context.DataBuffer, mode);

			context.GenerationHash[2] ^= 0xFF;

			// Assert:
			AssertValidationResult<2>(ValidationResult::Success, context.GenerationHash, notification);
		}

		//V2 account with V2 signature notification
		{
			// Arrange:
			TestContext<2> context(mode);
			model::SignatureNotification<2> notification(context.SignerKeyPair.publicKey(), context.Signature, 2, context.DataBuffer, mode);

			context.GenerationHash[2] ^= 0xFF;

			// Assert:
			AssertValidationResult<2>(ValidationResult::Success, context.GenerationHash, notification);
		}
	}
}}
