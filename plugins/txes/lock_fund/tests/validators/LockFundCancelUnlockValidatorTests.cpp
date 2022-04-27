/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/LockFundTestUtils.h"
#include "tests/test/nodeps/KeyTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS LockFundCancelUnlockValidatorTests


	DEFINE_COMMON_VALIDATOR_TESTS(LockFundCancelUnlock)

	namespace {
		template<typename TPrepareFunc, typename TModifier = void(cache::LockFundCacheDelta&)>
		void AssertValidationResult(ValidationResult expectedResult,
				const Height& unlockHeight,
				const BlockDuration& unlockCooldown,
				const uint16_t& maxMosaicsSize,
				const crypto::KeyPair& keypair,
				TPrepareFunc prepareAccount,
				TModifier modifyCache = [](cache::LockFundCacheDelta&){}) {
			model::LockFundCancelUnlockNotification<1> notification(keypair.publicKey(), unlockHeight);
			auto pValidator = CreateLockFundCancelUnlockValidator();
			test::AssertValidationResult(expectedResult,
				 notification,
				 std::move(pValidator),
				 unlockCooldown,
				 maxMosaicsSize,
				 keypair,
				 prepareAccount,
				 modifyCache);
		}
	}

	TEST(TEST_CLASS, SuccessWhenValidatingNotificationCancelingUnlock) {
		// Assert:
		auto keyPair = test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2);
		AssertValidationResult(test::Success_Result,
	   Height(116),
	   BlockDuration(15),
	   256,
	   keyPair,
	   [](state::AccountState& accountState){

	   },
	   [&keyPair](cache::LockFundCacheDelta& lockFundCache){
		 lockFundCache.insert(keyPair.publicKey(), Height(116), {{MosaicId(71), Amount(3)}});

	   });
	}
	TEST(TEST_CLASS, FailureWhenValidatingNotificationLockingMosaicCancellingNonExistantUnlock) {
		// Assert:
		auto keyPair = test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2);
		AssertValidationResult(Failure_LockFund_Request_Non_Existant,
			   Height(16),
			   BlockDuration(15),
			   256,
			   keyPair,
			   [](state::AccountState& accountState){

			   },
			   [&keyPair](cache::LockFundCacheDelta& lockFundCache){

			   });
	}

}}
