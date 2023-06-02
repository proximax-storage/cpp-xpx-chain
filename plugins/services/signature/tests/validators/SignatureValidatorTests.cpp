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
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"
#include "src/catapult/crypto/Signature.h"

namespace catapult { namespace validators {

	DEFINE_COMMON_VALIDATOR_TESTS(SignatureV1, GenerationHash())
	DEFINE_COMMON_VALIDATOR_TESTS(SignatureV2, GenerationHash())
	DEFINE_COMMON_VALIDATOR_TESTS(BlockSignatureV1, GenerationHash())
	DEFINE_COMMON_VALIDATOR_TESTS(BlockSignatureV2, GenerationHash())

#define TEST_CLASS SignatureValidatorTests

	namespace {

		const auto Max_Transaction_Lifetime = []() { return utils::TimeSpan::FromHours(2); }();
		constexpr auto Block_Time = Timestamp(8888);
		using ReplayProtectionMode = model::Replay::ReplayProtectionMode;

		template<bool IsBlock, uint32_t TSignatureNotificationVersion>
		struct SignatureNotificationTypeResolver {
			using Notification = model::SignatureNotification<TSignatureNotificationVersion>;
			static auto CreateValidatorImpl(const GenerationHash& hash, std::integral_constant<uint32_t, 1>) {
				return CreateSignatureV1Validator(hash);
			}
			static auto CreateValidatorImpl(const GenerationHash& hash, std::integral_constant<uint32_t, 2>) {
				return CreateSignatureV2Validator(hash);
			}

			static auto CreateValidator(const GenerationHash& hash) {
				return CreateValidatorImpl(hash, std::integral_constant<uint32_t, TSignatureNotificationVersion>{});
			}
		};

		template<uint32_t TSignatureNotificationVersion>
		struct SignatureNotificationTypeResolver<true, TSignatureNotificationVersion> {
			using Notification = model::BlockSignatureNotification<TSignatureNotificationVersion>;
			static auto CreateValidatorImpl(const GenerationHash& hash, std::integral_constant<uint32_t, 1>) {
				return CreateBlockSignatureV1Validator(hash);
			}
			static auto CreateValidatorImpl(const GenerationHash& hash, std::integral_constant<uint32_t, 2>) {
				return CreateBlockSignatureV2Validator(hash);
			}

			static auto CreateValidator(const GenerationHash& hash) {
				return CreateValidatorImpl(hash, std::integral_constant<uint32_t, TSignatureNotificationVersion>{});
			}
		};

		template<bool IsBlock, uint32_t TSignatureNotificationVersion>
		struct AssertValidationResult {
			static void Assertion(
					ValidationResult expectedResult,
					const GenerationHash& generationHash,
					const model::SignatureNotification<TSignatureNotificationVersion>& notification,
					uint32_t minimumVersion = 1,
					uint32_t activeVersion = 1);
		};


		template<bool IsBlock>
		struct AssertValidationResult<IsBlock, 1> {
			static void Assertion(
					ValidationResult expectedResult,
					const GenerationHash& generationHash,
					const typename SignatureNotificationTypeResolver<IsBlock, 1>::Notification& notification,
					uint32_t minimumVersion = 1,
					uint32_t activeVersion = 1) {
				// Arrange:
				auto pValidator = SignatureNotificationTypeResolver<IsBlock, 1>::CreateValidator(generationHash);

				// Act:
				auto result = test::ValidateNotification(*pValidator, notification);

				// Assert:
				EXPECT_EQ(expectedResult, result);
			}
		};

		template<bool IsBlock>
		struct AssertValidationResult<IsBlock, 2> {
			static void Assertion(
					ValidationResult expectedResult,
					const GenerationHash& generationHash,
					const typename SignatureNotificationTypeResolver<IsBlock, 2>::Notification& notification,
					uint32_t minimumVersion = 1,
					uint32_t activeVersion = 1) {
				// Arrange:
				auto pValidator = SignatureNotificationTypeResolver<IsBlock, 2>::CreateValidator(generationHash);

				test::MutableBlockchainConfiguration mutableConfig;
				mutableConfig.Network.MaxTransactionLifetime = Max_Transaction_Lifetime;
				mutableConfig.Network.EnableDeadlineValidation = true;
				mutableConfig.Network.AccountVersion = activeVersion;
				mutableConfig.Network.MinimumAccountVersion = minimumVersion;
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
		};


		template<uint32_t TSignerAccountVersion>
		struct TestContext {
		public:
			explicit TestContext(ReplayProtectionMode mode)
					: SignerKeyPair(test::GenerateKeyPair(TSignerAccountVersion))
					, GenerationHash(test::GenerateRandomByteArray<catapult::GenerationHash>())
					, DataBuffer(test::GenerateRandomVector(55)) {
				// when replay protection is enabled, data buffer should be prepended by generation hash
				if (ReplayProtectionMode::Enabled == mode)
					crypto::SignatureFeatureSolver::Sign(SignerKeyPair, { GenerationHash, DataBuffer }, Signature);
				else
					crypto::SignatureFeatureSolver::Sign(SignerKeyPair, DataBuffer, Signature);
			}

		public:
			crypto::KeyPair SignerKeyPair;
			catapult::GenerationHash GenerationHash;
			std::vector<uint8_t> DataBuffer;
			catapult::RawSignature Signature;
		};
	}

#define SIGNATURE_BOTH_BLOCK_TYPES_TEST(TEST_NAME) \
	template<bool IsBlock> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)();\
	TEST(TEST_CLASS, TEST_NAME##BLOCK) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<true>(); } \
	TEST(TEST_CLASS, TEST_NAME##NON_BLOCK) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<false>(); } \
	template<bool IsBlock> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()


#define ALL_REPLAY_PROTECTION_MODES_TEST(TEST_NAME) \
	template<bool IsBlock, ReplayProtectionMode Mode> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##BLOCK_ReplayProtectionEnabled) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<true, ReplayProtectionMode::Enabled>(); } \
	TEST(TEST_CLASS, TEST_NAME##NON_BLOCK_ReplayProtectionEnabled) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<false, ReplayProtectionMode::Enabled>(); } \
	TEST(TEST_CLASS, TEST_NAME##BLOCK_ReplayProtectionDisabled) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<true, ReplayProtectionMode::Disabled>(); } \
	TEST(TEST_CLASS, TEST_NAME##NON_BLOCK_ReplayProtectionDisabled) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<false, ReplayProtectionMode::Disabled>(); } \
	template<bool IsBlock, ReplayProtectionMode Mode> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	ALL_REPLAY_PROTECTION_MODES_TEST(SuccessWhenValidatingValidSignature) {

		// V1 account with deprecated V1 signature notification
		{
			// Arrange:
			TestContext<1> context(Mode);
			typename SignatureNotificationTypeResolver<IsBlock, 1>::Notification notification(context.SignerKeyPair.publicKey(), context.Signature, context.DataBuffer, Mode);

			// Assert:
			AssertValidationResult<IsBlock, 1>::Assertion(ValidationResult::Success, context.GenerationHash, notification);
		}

		// V1 account with V2 signature notification
		{
			// Arrange:
			TestContext<1> context(Mode);
			typename SignatureNotificationTypeResolver<IsBlock, 2>::Notification notification(context.SignerKeyPair.publicKey(), context.Signature, context.SignerKeyPair.derivationScheme(), context.DataBuffer, Mode);

			// Assert:
			AssertValidationResult<IsBlock, 2>::Assertion(ValidationResult::Success, context.GenerationHash, notification, 1, 2);
		}

		// V2 account with V2 signature notification
		{
			// Arrange:
			TestContext<2> context(Mode);
			typename SignatureNotificationTypeResolver<IsBlock, 2>::Notification notification(context.SignerKeyPair.publicKey(), context.Signature, context.SignerKeyPair.derivationScheme(), context.DataBuffer, Mode);

			// Assert:
			AssertValidationResult<IsBlock, 2>::Assertion(ValidationResult::Success, context.GenerationHash, notification, 1, 2);
		}
	}

	ALL_REPLAY_PROTECTION_MODES_TEST(FailureWhenSignatureIsAltered) {
		//V1 account with deprecated V1 signature notification
		{
			// Arrange:
			TestContext<1> context(Mode);
			typename SignatureNotificationTypeResolver<IsBlock, 1>::Notification notification(context.SignerKeyPair.publicKey(), context.Signature, context.DataBuffer, Mode);

			context.Signature[0] ^= 0xFF;

			// Assert:
			AssertValidationResult<IsBlock, 1>::Assertion(validators::VerifiableFailureResolver<IsBlock, Failure_Signature_Block_Not_Verifiable, Failure_Signature_Not_Verifiable>::ValidationResult, context.GenerationHash, notification);
		}

		//V1 account with V2 signature notification
		{
			// Arrange:
			TestContext<1> context(Mode);
			typename SignatureNotificationTypeResolver<IsBlock, 2>::Notification notification(context.SignerKeyPair.publicKey(), context.Signature, context.SignerKeyPair.derivationScheme(), context.DataBuffer, Mode);

			context.Signature[0] ^= 0xFF;

			// Assert:
			AssertValidationResult<IsBlock, 2>::Assertion(validators::VerifiableFailureResolver<IsBlock, Failure_Signature_Block_Not_Verifiable, Failure_Signature_Not_Verifiable>::ValidationResult, context.GenerationHash, notification, 1, 2);
		}

		//V2 account with V2 signature notification
		{
			// Arrange:
			TestContext<1> context(Mode);
			typename SignatureNotificationTypeResolver<IsBlock, 2>::Notification notification(context.SignerKeyPair.publicKey(), context.Signature, context.SignerKeyPair.derivationScheme(), context.DataBuffer, Mode);

			context.Signature[0] ^= 0xFF;

			// Assert:
			AssertValidationResult<IsBlock, 2>::Assertion(validators::VerifiableFailureResolver<IsBlock, Failure_Signature_Block_Not_Verifiable, Failure_Signature_Not_Verifiable>::ValidationResult, context.GenerationHash, notification, 1, 2);
		}
	}

	ALL_REPLAY_PROTECTION_MODES_TEST(FailureWhenDataIsAltered) {


		//V1 account with deprecated V1 signature notification
		{
			// Arrange:
			TestContext<1> context(Mode);
			typename SignatureNotificationTypeResolver<IsBlock, 1>::Notification notification(context.SignerKeyPair.publicKey(), context.Signature, context.DataBuffer, Mode);

			context.DataBuffer[10] ^= 0xFF;

			// Assert:
			AssertValidationResult<IsBlock, 1>::Assertion(validators::VerifiableFailureResolver<IsBlock, Failure_Signature_Block_Not_Verifiable, Failure_Signature_Not_Verifiable>::ValidationResult, context.GenerationHash, notification);
		}

		//V1 account with V2 signature notification
		{
			// Arrange:
			TestContext<1> context(Mode);
			typename SignatureNotificationTypeResolver<IsBlock, 2>::Notification notification(context.SignerKeyPair.publicKey(), context.Signature, context.SignerKeyPair.derivationScheme(), context.DataBuffer, Mode);

			context.DataBuffer[10] ^= 0xFF;

			// Assert:
			AssertValidationResult<IsBlock, 2>::Assertion(validators::VerifiableFailureResolver<IsBlock, Failure_Signature_Block_Not_Verifiable, Failure_Signature_Not_Verifiable>::ValidationResult, context.GenerationHash, notification, 1, 2);
		}

		//V2 account with V2 signature notification
		{
			// Arrange:
			TestContext<1> context(Mode);
			typename SignatureNotificationTypeResolver<IsBlock, 2>::Notification notification(context.SignerKeyPair.publicKey(), context.Signature, context.SignerKeyPair.derivationScheme(), context.DataBuffer, Mode);

			context.DataBuffer[10] ^= 0xFF;

			// Assert:
			AssertValidationResult<IsBlock, 2>::Assertion(validators::VerifiableFailureResolver<IsBlock, Failure_Signature_Block_Not_Verifiable, Failure_Signature_Not_Verifiable>::ValidationResult, context.GenerationHash, notification, 1, 2);
		}
	}

	SIGNATURE_BOTH_BLOCK_TYPES_TEST(FailureWhenGenerationHashIsAltered_ReplayProtectionEnabled) {

		auto mode = ReplayProtectionMode::Enabled;

		//V1 account with deprecated V1 signature notification
		{
			// Arrange:
			TestContext<1> context(mode);
			typename SignatureNotificationTypeResolver<IsBlock, 1>::Notification notification(context.SignerKeyPair.publicKey(), context.Signature, context.DataBuffer, mode);

			context.GenerationHash[2] ^= 0xFF;

			// Assert:
			AssertValidationResult<IsBlock, 1>::Assertion(validators::VerifiableFailureResolver<IsBlock, Failure_Signature_Block_Not_Verifiable, Failure_Signature_Not_Verifiable>::ValidationResult, context.GenerationHash, notification);
		}

		//V1 account with V2 signature notification
		{
			// Arrange:
			TestContext<1> context(mode);
			typename SignatureNotificationTypeResolver<IsBlock, 2>::Notification notification(context.SignerKeyPair.publicKey(), context.Signature, context.SignerKeyPair.derivationScheme(), context.DataBuffer, mode);

			context.GenerationHash[2] ^= 0xFF;

			// Assert:
			AssertValidationResult<IsBlock, 2>::Assertion(validators::VerifiableFailureResolver<IsBlock, Failure_Signature_Block_Not_Verifiable, Failure_Signature_Not_Verifiable>::ValidationResult, context.GenerationHash, notification, 1, 2);
		}

		//V2 account with V2 signature notification
		{
			// Arrange:
			TestContext<2> context(mode);
			typename SignatureNotificationTypeResolver<IsBlock, 2>::Notification notification(context.SignerKeyPair.publicKey(), context.Signature, context.SignerKeyPair.derivationScheme(), context.DataBuffer, mode);

			context.GenerationHash[2] ^= 0xFF;

			// Assert:
			AssertValidationResult<IsBlock, 2>::Assertion(validators::VerifiableFailureResolver<IsBlock, Failure_Signature_Block_Not_Verifiable, Failure_Signature_Not_Verifiable>::ValidationResult, context.GenerationHash, notification, 1, 2);
		}
	}

	SIGNATURE_BOTH_BLOCK_TYPES_TEST(SuccessWhenGenerationHashIsAltered_ReplayProtectionDisabled) {

		auto mode = ReplayProtectionMode::Disabled;

		//V1 account with deprecated V1 signature notification
		{
			// Arrange:
			TestContext<1> context(mode);
			typename SignatureNotificationTypeResolver<IsBlock, 1>::Notification notification(context.SignerKeyPair.publicKey(), context.Signature, context.DataBuffer, mode);

			context.GenerationHash[2] ^= 0xFF;

			// Assert:
			AssertValidationResult<IsBlock, 1>::Assertion(ValidationResult::Success, context.GenerationHash, notification);
		}

		//V1 account with V2 signature notification
		{
			// Arrange:
			TestContext<1> context(mode);
			typename SignatureNotificationTypeResolver<IsBlock, 2>::Notification notification(context.SignerKeyPair.publicKey(), context.Signature, context.SignerKeyPair.derivationScheme(), context.DataBuffer, mode);

			context.GenerationHash[2] ^= 0xFF;

			// Assert:
			AssertValidationResult<IsBlock, 2>::Assertion(ValidationResult::Success, context.GenerationHash, notification, 1, 2);
		}

		//V2 account with V2 signature notification
		{
			// Arrange:
			TestContext<2> context(mode);
			typename SignatureNotificationTypeResolver<IsBlock, 2>::Notification notification(context.SignerKeyPair.publicKey(), context.Signature, context.SignerKeyPair.derivationScheme(), context.DataBuffer, mode);

			context.GenerationHash[2] ^= 0xFF;

			// Assert:
			AssertValidationResult<IsBlock, 2>::Assertion(ValidationResult::Success, context.GenerationHash, notification, 1, 2);
		}
	}

	SIGNATURE_BOTH_BLOCK_TYPES_TEST(FailureWhenVersionHasBeenDeprecated) {

		auto mode = ReplayProtectionMode::Disabled;


		//V1 account with V2 signature notification
		{
			// Arrange:
			TestContext<1> context(mode);
			typename SignatureNotificationTypeResolver<IsBlock, 2>::Notification notification(context.SignerKeyPair.publicKey(), context.Signature, context.SignerKeyPair.derivationScheme(), context.DataBuffer, mode);

			// Assert:
			AssertValidationResult<IsBlock, 2>::Assertion(validators::VerifiableFailureResolver<IsBlock, Failure_Signature_Block_Invalid_Version, Failure_Signature_Invalid_Version>::ValidationResult, context.GenerationHash, notification, 2, 2);
		}
	}

	SIGNATURE_BOTH_BLOCK_TYPES_TEST(SuccessWhenVersionHasNotBeenDeprecated) {

		auto mode = ReplayProtectionMode::Disabled;


		//V1 account with V2 signature notification
		{
			// Arrange:
			TestContext<1> context(mode);
			typename SignatureNotificationTypeResolver<IsBlock, 2>::Notification notification(context.SignerKeyPair.publicKey(), context.Signature, context.SignerKeyPair.derivationScheme(), context.DataBuffer, mode);

			// Assert:
			AssertValidationResult<IsBlock, 2>::Assertion(ValidationResult::Success, context.GenerationHash, notification, 1, 2);
		}
	}
}}
