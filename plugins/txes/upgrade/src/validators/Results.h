/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#ifndef CUSTOM_RESULT_DEFINITION
#include "catapult/validators/ValidationResult.h"

namespace catapult { namespace validators {

#endif
/// Defines a blockchain upgrade validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_CATAPULT_UPGRADE_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, BlockchainUpgrade, DESCRIPTION, CODE, None)

	/// Validation failed because the signer is not nemesis account.
	DEFINE_CATAPULT_UPGRADE_RESULT(Invalid_Signer, 1);

	/// Validation failed because the upgrade period less than allowed.
	DEFINE_CATAPULT_UPGRADE_RESULT(Upgrade_Period_Too_Low, 2);

	/// Validation failed because the upgrade already in-progress.
	DEFINE_CATAPULT_UPGRADE_RESULT(Redundant, 3);

	/// Validation failed because the current blockchain version is invalid.
	DEFINE_CATAPULT_UPGRADE_RESULT(Invalid_Current_Version, 4);

	/// Validation failed because plugin configuration data is malformed.
	DEFINE_CATAPULT_UPGRADE_RESULT(Plugin_Config_Malformed, 5);

	/// Validation failed because this public key already belongs to an account.
	DEFINE_CATAPULT_UPGRADE_RESULT(Account_Duplicate, 6);

	/// Validation failed because the signer cannot be upgraded.
	DEFINE_CATAPULT_UPGRADE_RESULT(Account_Not_Upgradable, 7);

	/// Validation failed because the signer cannot be upgraded.
	DEFINE_CATAPULT_UPGRADE_RESULT(Account_Non_Existant, 8);

	/// Validation failed because the signer cannot be upgraded.
	DEFINE_CATAPULT_UPGRADE_RESULT(Account_Version_Not_Allowed, 9);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
