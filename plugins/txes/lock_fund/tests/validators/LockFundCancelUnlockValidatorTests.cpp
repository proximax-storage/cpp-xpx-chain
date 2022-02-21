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
	   Height(16),
	   BlockDuration(15),
	   256,
	   keyPair,
	   [](state::AccountState& accountState){

	   },
	   [&keyPair](cache::LockFundCacheDelta& lockFundCache){
		 lockFundCache.insert(keyPair.publicKey(), Height(16), {{MosaicId(71), Amount(3)}});

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
