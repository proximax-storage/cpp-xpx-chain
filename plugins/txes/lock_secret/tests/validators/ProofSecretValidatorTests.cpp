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
#include "src/model/LockHashUtils.h"
#include "src/config/SecretLockConfiguration.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS ProofSecretValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(ProofSecret)

	namespace {
		struct NotificationBuilder {
		public:
			NotificationBuilder(model::LockHashAlgorithm algorithm = model::LockHashAlgorithm::Op_Sha3_256)
					: m_algorithm(algorithm) {
				setProofSize(50);
				test::FillWithRandomData(m_secret);
			}

		public:
			auto notification() {
				return model::ProofSecretNotification<1>(m_algorithm, m_secret, m_proof);
			}

			void setProofSize(size_t proofSize) {
				m_proof.resize(proofSize);
				test::FillWithRandomData(m_proof);
			}

			void setValidHash() {
				m_secret = model::CalculateHash(m_algorithm, m_proof);
			}

		private:
			model::LockHashAlgorithm m_algorithm;
			std::vector<uint8_t> m_proof;
			Hash256 m_secret;
		};

		auto CreateConfigHolder() {
			auto pluginConfig = config::SecretLockConfiguration::Uninitialized();
			pluginConfig.MinProofSize = 10;
			pluginConfig.MaxProofSize = 100;
			auto networkConfig = model::NetworkConfiguration::Uninitialized();
			networkConfig.SetPluginConfiguration(pluginConfig);
			return config::CreateMockConfigurationHolder(networkConfig);
		}

		void AssertFailureIfHashAlgorithmIsNotSupported(model::LockHashAlgorithm lockHashAlgorithm) {
			// Arrange:
			NotificationBuilder notificationBuilder(lockHashAlgorithm);
			auto pConfigHolder = CreateConfigHolder();
			auto cache = test::CreateEmptyCatapultCache(pConfigHolder->Config());
			auto pValidator = CreateProofSecretValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notificationBuilder.notification(), cache, pConfigHolder->Config());

			// Assert:
			EXPECT_EQ(Failure_LockSecret_Hash_Not_Implemented, result)
					<< "hash algorithm: " << utils::to_underlying_type(lockHashAlgorithm);
		}
	}

	TEST(TEST_CLASS, FailureWhenHashAlgorithmIsNotSupported) {
		using model::LockHashAlgorithm;

		// Assert:
		auto unsupportedAlgorithm = static_cast<LockHashAlgorithm>(utils::to_underlying_type(LockHashAlgorithm::Op_Hash_256) + 1);
		AssertFailureIfHashAlgorithmIsNotSupported(unsupportedAlgorithm);
	}

	TEST(TEST_CLASS, FailureWhenSecretDoesNotMatchProof) {
		NotificationBuilder notificationBuilder;
		auto pConfigHolder = CreateConfigHolder();
		auto cache = test::CreateEmptyCatapultCache(pConfigHolder->Config());
		auto pValidator = CreateProofSecretValidator();

		// Act:
		auto result = test::ValidateNotification(*pValidator, notificationBuilder.notification(), cache, pConfigHolder->Config());

		// Assert:
		EXPECT_EQ(Failure_LockSecret_Secret_Mismatch, result);
	}

	namespace {
		void AssertSuccessIfSecretMatchesProof(model::LockHashAlgorithm algorithm) {
			// Arrange:
			NotificationBuilder notificationBuilder(algorithm);
			notificationBuilder.setValidHash();
			auto pConfigHolder = CreateConfigHolder();
			auto cache = test::CreateEmptyCatapultCache(pConfigHolder->Config());
			auto pValidator = CreateProofSecretValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notificationBuilder.notification(), cache, pConfigHolder->Config());

			// Assert:
			EXPECT_EQ(ValidationResult::Success, result);
		}
	}

	TEST(TEST_CLASS, SuccessWhenSecretMatchesProof_Sha3) {
		// Assert:
		AssertSuccessIfSecretMatchesProof(model::LockHashAlgorithm::Op_Sha3_256);
	}

	TEST(TEST_CLASS, SuccessWhenSecretMatchesProof_Keccak) {
		// Assert:
		AssertSuccessIfSecretMatchesProof(model::LockHashAlgorithm::Op_Keccak_256);
	}

	TEST(TEST_CLASS, SuccessWhenSecretMatchesProof_Bitcoin160) {
		// Assert:
		AssertSuccessIfSecretMatchesProof(model::LockHashAlgorithm::Op_Hash_160);
	}

	TEST(TEST_CLASS, SuccessWhenSecretMatchesProof_Sha256Double) {
		// Assert:
		AssertSuccessIfSecretMatchesProof(model::LockHashAlgorithm::Op_Hash_256);
	}

	namespace {
		void AssertProofSize(ValidationResult expectedResult, size_t proofSize) {
			// Arrange:
			NotificationBuilder notificationBuilder;
			notificationBuilder.setProofSize(proofSize);
			notificationBuilder.setValidHash();
			auto pConfigHolder = CreateConfigHolder();
			auto cache = test::CreateEmptyCatapultCache(pConfigHolder->Config());
			auto pValidator = CreateProofSecretValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notificationBuilder.notification(), cache, pConfigHolder->Config());

			// Assert:
			EXPECT_EQ(expectedResult, result) << "proof size: " << proofSize;
		}
	}

	TEST(TEST_CLASS, FailureWhenProofIsOutOfBounds) {
		// Assert: minimum size is 10, maximum is 100, so all should fail
		for (auto proofSize : { 3u, 9u, 101u, 105u })
			AssertProofSize(Failure_LockSecret_Proof_Size_Out_Of_Bounds, proofSize);
	}

	TEST(TEST_CLASS, SuccessWhenProofIsWithinBounds) {
		// Assert: minimum size is 10, maximum is 100, so all should succeed
		for (auto proofSize : { 10u, 40u, 100u })
			AssertProofSize(ValidationResult::Success, proofSize);
	}
}}
