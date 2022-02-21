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

	namespace {
		template<typename TPrepareFunc, typename TModifier = void(cache::LockFundCacheDelta&)>
		void AssertValidationResult(ValidationResult expectedResult,
									const BlockDuration& blocksUntilUnlock,
									const std::vector<model::UnresolvedMosaic>& mosaics,
									model::LockFundAction lockFundAction,
									const BlockDuration& unlockCooldown,
									const uint16_t& maxMosaicsSize,
									const crypto::KeyPair& keypair,
									TPrepareFunc prepareAccount,
									TModifier modifyCache = [](cache::LockFundCacheDelta&){}) {
			model::LockFundTransferNotification<1> notification(keypair.publicKey(), mosaics.size(), blocksUntilUnlock, mosaics.data(), lockFundAction);
			auto pValidator = CreateLockFundTransferValidator();
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
#define TEST_CLASS LockFundTransferValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(LockFundTransfer)

	TEST(TEST_CLASS, FailureWhenValidatingLockNotificationWithZeroMosaics) {
		// Assert:
		AssertValidationResult(Failure_LockFund_Zero_Amount, BlockDuration(1), {}, model::LockFundAction::Lock, BlockDuration(1), 256, test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2), [](auto){});
	}

	TEST(TEST_CLASS, FailureWhenValidatingUnlockNotificationWithZeroMosaics) {
		// Assert:
		AssertValidationResult(Failure_LockFund_Zero_Amount, BlockDuration(1), {}, model::LockFundAction::Unlock, BlockDuration(1), 256, test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2), [](auto){});
	}

	TEST(TEST_CLASS, SuccessWhenValidatingNotificationLockingOneMosaic) {
		// Assert:
		AssertValidationResult(test::Success_Result,
		   BlockDuration(1),
		   { { UnresolvedMosaicId(71), Amount(5) } },
		   model::LockFundAction::Lock,
		   BlockDuration(1),
		   256,
		   test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2),
		   [](state::AccountState& accountState){
			   accountState.Balances.credit(MosaicId(71), Amount(5));
   			});
	}

	TEST(TEST_CLASS, SuccessWhenValidatingNotificationUnlockingOneMosaic) {
		// Assert:
		AssertValidationResult(test::Success_Result,
		   BlockDuration(1),
		   { { UnresolvedMosaicId(71), Amount(5) } },
		   model::LockFundAction::Unlock,
		   BlockDuration(1),
		   256,
		   test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2),
		   [](state::AccountState& accountState){
			 accountState.Balances.credit(MosaicId(71), Amount(5));
			 accountState.Balances.lock(MosaicId(71), Amount(5));
	   });
	}

	TEST(TEST_CLASS, SuccessWhenValidatingNotificationLockingMultipleOrderedMosaic) {
		// Assert:
		AssertValidationResult(test::Success_Result,
		   BlockDuration(1),
		   { { UnresolvedMosaicId(71), Amount(5) }, { UnresolvedMosaicId(182), Amount(4) }},
		   model::LockFundAction::Lock,
		   BlockDuration(1),
		   256,
		   test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2),
		   [](state::AccountState& accountState){
			 accountState.Balances.credit(MosaicId(71), Amount(5));
			 accountState.Balances.credit(MosaicId(182), Amount(4));
		   });
	}

	TEST(TEST_CLASS, SuccessWhenValidatingNotificationUnlockingMultipleOrderedMosaic) {
		// Assert:
		AssertValidationResult(test::Success_Result,
		   BlockDuration(1),
		   { { UnresolvedMosaicId(71), Amount(5) }, { UnresolvedMosaicId(182), Amount(4) }},
		   model::LockFundAction::Unlock,
		   BlockDuration(1),
		   256,
		   test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2),
		   [](state::AccountState& accountState){
			 accountState.Balances.credit(MosaicId(71), Amount(5));
			 accountState.Balances.credit(MosaicId(182), Amount(4));
			 accountState.Balances.lock(MosaicId(71), Amount(5));
			 accountState.Balances.lock(MosaicId(182), Amount(4));
		   });
	}

TEST(TEST_CLASS, FailureWhenValidatingNotificationLockingMultipleOutOfOrderMosaics) {
	// Assert:
	// - first and second are out of order
	AssertValidationResult(Failure_LockFund_Out_Of_Order_Mosaics,
	   BlockDuration(1),
						   { { UnresolvedMosaicId(200), Amount(5) }, { UnresolvedMosaicId(71), Amount(1) }, { UnresolvedMosaicId(182), Amount(4) } },
	   model::LockFundAction::Lock,
	   BlockDuration(1),
	   256,
	   test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2),
	   [](state::AccountState& accountState){
		 accountState.Balances.credit(MosaicId(200), Amount(5));
		 accountState.Balances.credit(MosaicId(71), Amount(5));
		 accountState.Balances.credit(MosaicId(182), Amount(5));
	   });

	// - second and third are out of order
	AssertValidationResult(Failure_LockFund_Out_Of_Order_Mosaics,
	   BlockDuration(1),
	   { { UnresolvedMosaicId(71), Amount(5) }, { UnresolvedMosaicId(200), Amount(1) }, { UnresolvedMosaicId(182), Amount(4) } },
	   model::LockFundAction::Lock,
	   BlockDuration(1),
	   256,
	   test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2),
	   [](state::AccountState& accountState){
		 accountState.Balances.credit(MosaicId(200), Amount(5));
		 accountState.Balances.credit(MosaicId(71), Amount(5));
		 accountState.Balances.credit(MosaicId(182), Amount(5));
	   });
	}

	TEST(TEST_CLASS, FailureWhenValidatingNotificationUnlockingMultipleOutOfOrderMosaics) {
		// Assert:
		// - first and second are out of order
		AssertValidationResult(Failure_LockFund_Out_Of_Order_Mosaics,
		   BlockDuration(1),
		   { { UnresolvedMosaicId(200), Amount(5) }, { UnresolvedMosaicId(71), Amount(1) }, { UnresolvedMosaicId(182), Amount(4) } },
		   model::LockFundAction::Unlock,
		   BlockDuration(1),
		   256,
		   test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2),
		   [](state::AccountState& accountState){
			 accountState.Balances.credit(MosaicId(200), Amount(5));
			 accountState.Balances.credit(MosaicId(71), Amount(5));
			 accountState.Balances.credit(MosaicId(182), Amount(5));
			 accountState.Balances.lock(MosaicId(200), Amount(5));
			 accountState.Balances.lock(MosaicId(71), Amount(5));
			 accountState.Balances.lock(MosaicId(182), Amount(5));
		   });

		// - second and third are out of order
		AssertValidationResult(Failure_LockFund_Out_Of_Order_Mosaics,
		   BlockDuration(1),
		   { { UnresolvedMosaicId(71), Amount(5) }, { UnresolvedMosaicId(200), Amount(1) }, { UnresolvedMosaicId(182), Amount(4) } },
		   model::LockFundAction::Unlock,
		   BlockDuration(1),
		   256,
		   test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2),
		   [](state::AccountState& accountState){
			 accountState.Balances.credit(MosaicId(200), Amount(5));
			 accountState.Balances.credit(MosaicId(71), Amount(5));
			 accountState.Balances.credit(MosaicId(182), Amount(5));
			 accountState.Balances.lock(MosaicId(200), Amount(5));
			 accountState.Balances.lock(MosaicId(71), Amount(5));
			 accountState.Balances.lock(MosaicId(182), Amount(5));
		   });
	}

	TEST(TEST_CLASS, FailureWhenValidatingNotificationUnlockingMultipleTransfersOfSameMosaic) {
		// Assert: create a transaction with multiple (in order) transfers for the same mosaic
		AssertValidationResult(Failure_LockFund_Out_Of_Order_Mosaics,
		   BlockDuration(1),
		   { { UnresolvedMosaicId(71), Amount(5) }, { UnresolvedMosaicId(71), Amount(1) }, { UnresolvedMosaicId(182), Amount(4) } },
		   model::LockFundAction::Unlock,
		   BlockDuration(1),
		   256,
		   test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2),
		   [](state::AccountState& accountState){
			 accountState.Balances.credit(MosaicId(71), Amount(5));
			 accountState.Balances.credit(MosaicId(182), Amount(5));
			 accountState.Balances.lock(MosaicId(71), Amount(5));
			 accountState.Balances.lock(MosaicId(182), Amount(5));
		   });
	}

	TEST(TEST_CLASS, FailureWhenValidatingNotificationLockingMultipleTransfersOfSameMosaic) {
		// Assert: create a transaction with multiple (in order) transfers for the same mosaic
		AssertValidationResult(Failure_LockFund_Out_Of_Order_Mosaics,
		   BlockDuration(1),
		   { { UnresolvedMosaicId(71), Amount(5) }, { UnresolvedMosaicId(71), Amount(1) }, { UnresolvedMosaicId(182), Amount(4) } },
		   model::LockFundAction::Lock,
		   BlockDuration(1),
		   256,
		   test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2),
		   [](state::AccountState& accountState){
			 accountState.Balances.credit(MosaicId(71), Amount(5));
			 accountState.Balances.credit(MosaicId(182), Amount(5));
		   });
	}

	TEST(TEST_CLASS, FailureWhenValidatingNotificationLockingOneMosaicZeroAmount) {
		AssertValidationResult(Failure_LockFund_Zero_Amount,
		   BlockDuration(1),
		   { { UnresolvedMosaicId(71), Amount(0) }},
		   model::LockFundAction::Lock,
		   BlockDuration(1),
		   256,
		   test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2),
		   [](state::AccountState& accountState){

		   });
	}

	TEST(TEST_CLASS, FailureWhenValidatingNotificationUnlockingOneMosaicZeroAmount) {
		AssertValidationResult(Failure_LockFund_Zero_Amount,
			   BlockDuration(1),
			   { { UnresolvedMosaicId(71), Amount(0) }},
			   model::LockFundAction::Unlock,
			   BlockDuration(1),
			   256,
			   test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2),
			   [](state::AccountState& accountState){

			   });
	}

	TEST(TEST_CLASS, FailureWhenValidatingNotificationWithTwoMosaicsWhenMaxMosaicsSizeIsOne) {
		// Assert:

		AssertValidationResult(Failure_LockFund_Too_Many_Mosaics,
			   BlockDuration(1),
			   {{ UnresolvedMosaicId(200), Amount(5) }, { UnresolvedMosaicId(71), Amount(1) }},
			   model::LockFundAction::Unlock,
			   BlockDuration(1),
			   1,
			   test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2),
			   [](state::AccountState& accountState){

			   });
	}

	TEST(TEST_CLASS, FailureWhenValidatingNotificationLockingTwoMosaicsWhenAccountIsLocked) {
		// Assert:

		AssertValidationResult(Failure_LockFund_Invalid_Sender,
		   BlockDuration(1),
		   {{ UnresolvedMosaicId(71), Amount(5) }, { UnresolvedMosaicId(200), Amount(1) }},
		   model::LockFundAction::Lock,
		   BlockDuration(1),
		   256,
		   test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2),
		   [](state::AccountState& accountState){
				accountState.AccountType = state::AccountType::Locked;
		   });
	}

	TEST(TEST_CLASS, FailureWhenValidatingNotificationUnlockingTwoMosaicsWhenAccountIsLocked) {
		// Assert:

		AssertValidationResult(Failure_LockFund_Invalid_Sender,
		   BlockDuration(1),
		   {{ UnresolvedMosaicId(71), Amount(5) }, { UnresolvedMosaicId(200), Amount(1) }},
		   model::LockFundAction::Unlock,
		   BlockDuration(1),
		   256,
		   test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2),
		   [](state::AccountState& accountState){
			 accountState.AccountType = state::AccountType::Locked;
		   });
	}

	TEST(TEST_CLASS, FailureWhenValidatingNotificationUnlockingTwoMosaicsWithSmallerDurationThanMinimum) {
		// Assert:

		AssertValidationResult(Failure_LockFund_Duration_Smaller_Than_Configured,
		   BlockDuration(1),
		   {{ UnresolvedMosaicId(71), Amount(5) }, { UnresolvedMosaicId(200), Amount(1) }},
		   model::LockFundAction::Unlock,
		   BlockDuration(15),
		   256,
		   test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2),
		   [](state::AccountState& accountState){

		   });
	}

	TEST(TEST_CLASS, FailureWhenValidatingNotificationLockingTwoMosaicsWithSmallerDurationThanMinimum) {
		// Assert:

		AssertValidationResult(Failure_LockFund_Duration_Smaller_Than_Configured,
		   BlockDuration(1),
		   {{ UnresolvedMosaicId(71), Amount(5) }, { UnresolvedMosaicId(200), Amount(1) }},
		   model::LockFundAction::Lock,
		   BlockDuration(15),
		   256,
		   test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2),
		   [](state::AccountState& accountState){

		   });
	}

	TEST(TEST_CLASS, FailureWhenValidatingNotificationLockingMosaicWithoutEnoughFundsZeroFunds) {
		// Assert:

		AssertValidationResult(Failure_LockFund_Not_Enough_Funds,
		   BlockDuration(16),
		   {{ UnresolvedMosaicId(71), Amount(5) }},
		   model::LockFundAction::Lock,
		   BlockDuration(15),
		   256,
		   test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2),
		   [](state::AccountState& accountState){

		   });
	}

	TEST(TEST_CLASS, FailureWhenValidatingNotificationLockingMosaicWithoutEnoughFundsInsufficient) {
		// Assert:

		AssertValidationResult(Failure_LockFund_Not_Enough_Funds,
		   BlockDuration(16),
		   {{ UnresolvedMosaicId(71), Amount(5) }},
		   model::LockFundAction::Lock,
		   BlockDuration(15),
		   256,
		   test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2),
		   [](state::AccountState& accountState){
			 accountState.Balances.credit(MosaicId(71), Amount(3));
		   });
	}

	TEST(TEST_CLASS, FailureWhenValidatingNotificationUnlockingMosaicWithoutEnoughFundsZeroFunds) {
		// Assert:

		AssertValidationResult(Failure_LockFund_Not_Enough_Funds,
		   BlockDuration(16),
		   {{ UnresolvedMosaicId(71), Amount(5) }},
		   model::LockFundAction::Unlock,
		   BlockDuration(15),
		   256,
		   test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2),
		   [](state::AccountState& accountState){

		   });
	}

	TEST(TEST_CLASS, FailureWhenValidatingNotificationUnlockingMosaicWithoutEnoughFundsInsufficient) {
		// Assert:

		AssertValidationResult(Failure_LockFund_Not_Enough_Funds,
		   BlockDuration(16),
		   {{ UnresolvedMosaicId(71), Amount(5) }},
		   model::LockFundAction::Unlock,
		   BlockDuration(15),
		   256,
		   test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2),
		   [](state::AccountState& accountState){
			 accountState.Balances.credit(MosaicId(71), Amount(5));
			 accountState.Balances.lock(MosaicId(71), Amount(3));
		   });
	}

	TEST(TEST_CLASS, FailureWhenValidatingNotificationUnlockingMosaicWithoutEnoughFundsExistingRequest) {
		// Assert:
		auto keyPair = test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2);
		AssertValidationResult(Failure_LockFund_Not_Enough_Funds,
		   BlockDuration(16),
		   {{ UnresolvedMosaicId(71), Amount(5) }},
		   model::LockFundAction::Unlock,
		   BlockDuration(15),
		   256,
		   keyPair,
		   [](state::AccountState& accountState){
			 accountState.Balances.credit(MosaicId(71), Amount(5));
			 accountState.Balances.lock(MosaicId(71), Amount(5));
		   },
			[&keyPair](cache::LockFundCacheDelta& lockFundCache){
				lockFundCache.insert(keyPair.publicKey(), Height(30), {{MosaicId(71), Amount(3)}});
		});
	}

	TEST(TEST_CLASS, FailureWhenValidatingNotificationUnlockingMosaicWithoutEnoughFundsExistingMultipleRequests) {
		// Assert:
		auto keyPair = test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2);
		AssertValidationResult(Failure_LockFund_Not_Enough_Funds,
		   BlockDuration(16),
		   {{ UnresolvedMosaicId(71), Amount(2) }},
		   model::LockFundAction::Unlock,
		   BlockDuration(15),
		   256,
		   keyPair,
		   [](state::AccountState& accountState){
			 accountState.Balances.credit(MosaicId(71), Amount(5));
			 accountState.Balances.lock(MosaicId(71), Amount(5));
		   },
		   [&keyPair](cache::LockFundCacheDelta& lockFundCache){
			 lockFundCache.insert(keyPair.publicKey(), Height(30), {{MosaicId(71), Amount(3)}});
			 lockFundCache.insert(keyPair.publicKey(), Height(30), {{MosaicId(71), Amount(1)}});
		   });
	}

	TEST(TEST_CLASS, FailureWhenValidatingNotificationUnlockingMosaicWithDuplicateRecord) {
		// Assert:
		auto keyPair = test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2);
		AssertValidationResult(Failure_LockFund_Duplicate_Record,
		   BlockDuration(16),
		   {{ UnresolvedMosaicId(71), Amount(2) }},
		   model::LockFundAction::Unlock,
		   BlockDuration(15),
		   256,
		   keyPair,
		   [](state::AccountState& accountState){
			 accountState.Balances.credit(MosaicId(71), Amount(5));
			 accountState.Balances.lock(MosaicId(71), Amount(5));
		   },
		   [&keyPair](cache::LockFundCacheDelta& lockFundCache){
			 lockFundCache.insert(keyPair.publicKey(), Height(16), {{MosaicId(71), Amount(3)}});

		   });
	}
	TEST(TEST_CLASS, FailureWhenValidatingNotificationLockingMosaicWithDuplicateRecord) {
		// Assert:
		auto keyPair = test::GenerateKeyPair(DerivationScheme::Ed25519_Sha2);
		AssertValidationResult(Failure_LockFund_Duplicate_Record,
		   BlockDuration(16),
		   {{ UnresolvedMosaicId(71), Amount(2) }},
		   model::LockFundAction::Lock,
		   BlockDuration(15),
		   256,
		   keyPair,
		   [](state::AccountState& accountState){
			 accountState.Balances.credit(MosaicId(71), Amount(5));
			 accountState.Balances.lock(MosaicId(71), Amount(5));
		   },
		   [&keyPair](cache::LockFundCacheDelta& lockFundCache){
			 lockFundCache.insert(keyPair.publicKey(), Height(16), {{MosaicId(71), Amount(3)}});

		   });
	}

}}
