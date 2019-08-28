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
/// Defines a network config validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_CATAPULT_CONFIG_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, NetworkConfig, DESCRIPTION, CODE, None)

	/// Validation failed because the signer is not nemesis account.
	DEFINE_CATAPULT_CONFIG_RESULT(Invalid_Signer, 1);

	/// Validation failed because the blockchain configuration data exceeded the limit.
	DEFINE_CATAPULT_CONFIG_RESULT(BlockChain_Config_Too_Large, 2);

	/// Validation failed because there is another config change at the height.
	DEFINE_CATAPULT_CONFIG_RESULT(Config_Redundant, 3);

	/// Validation failed because blockchain configuration data is malformed.
	DEFINE_CATAPULT_CONFIG_RESULT(BlockChain_Config_Malformed, 4);

	/// Validation failed because plugin configuration data is malformed.
	DEFINE_CATAPULT_CONFIG_RESULT(Plugin_Config_Malformed, 5);

	/// Validation failed because the supported entity versions configuration data exceeded the limit.
	DEFINE_CATAPULT_CONFIG_RESULT(SupportedEntityVersions_Config_Too_Large, 6);

	/// Validation failed because the supported entity versions configuration data is malformed.
	DEFINE_CATAPULT_CONFIG_RESULT(SupportedEntityVersions_Config_Malformed, 7);

	/// Validation failed because the supported entity versions configuration data has no versions of the network config transaction.
	DEFINE_CATAPULT_CONFIG_RESULT(Network_Config_Trx_Cannot_Be_Unsupported, 8);

	/// Validation failed because some plugin config missing.
	DEFINE_CATAPULT_CONFIG_RESULT(Plugin_Config_Missing, 9);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
