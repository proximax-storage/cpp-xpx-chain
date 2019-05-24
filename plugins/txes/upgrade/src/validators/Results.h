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
/// Defines a catapult upgrade validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_CATAPULT_UPGRADE_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, CatapultUpgrade, DESCRIPTION, CODE, None)

	/// Validation failed because the signer is not nemesis account.
	DEFINE_CATAPULT_UPGRADE_RESULT(Invalid_Signer, 1);

	/// Validation failed because the upgrade period less than allowed.
	DEFINE_CATAPULT_UPGRADE_RESULT(Upgrade_Period_Too_Low, 2);

	/// Validation failed because the upgrade already in-progress.
	DEFINE_CATAPULT_UPGRADE_RESULT(Redundant, 3);

	/// Validation failed because catapult version is invalid.
	DEFINE_CATAPULT_UPGRADE_RESULT(Invalid_Catapult_Version, 4);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
