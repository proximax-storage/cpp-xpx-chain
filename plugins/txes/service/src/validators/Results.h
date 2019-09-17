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
/// Defines a drive validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_SERVICE_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, Service, DESCRIPTION, CODE, None)

	/// Validation failed because the drive key is not a multisig.
	DEFINE_SERVICE_RESULT(Drive_Key_Is_Not_Multisig, 1);

	/// Validation failed because the drive already exists.
	DEFINE_SERVICE_RESULT(Drive_Alredy_Exists, 2);

	/// Validation failed because the drive doesn't exist.
	DEFINE_SERVICE_RESULT(Drive_Doesnt_Exist, 3);

	/// Validation failed because drive duration is zero.
	DEFINE_SERVICE_RESULT(Drive_Invalid_Duration, 4);

	/// Validation failed because drive prolongation duration is zero.
	DEFINE_SERVICE_RESULT(Drive_Invalid_Prolongation_Duration, 5);

	/// Validation failed because duration is invalid.
	DEFINE_SERVICE_RESULT(Drive_Invalid_Size, 6);

	/// Validation failed because duration is invalid.
	DEFINE_SERVICE_RESULT(Drive_Invalid_Replicas, 7);

	/// Validation failed because plugin configuration data is malformed.
	DEFINE_SERVICE_RESULT(Plugin_Config_Malformed, 8);

	/// Validation failed because a single mosaic is not single.
	DEFINE_SERVICE_RESULT(Multiple_Mosaics, 9);

	/// Validation failed because mosaic amount is zero.
	DEFINE_SERVICE_RESULT(Zero_Amount, 10);

	/// Validation failed because drive deposit is too small.
	DEFINE_SERVICE_RESULT(Drive_Deposit_Too_Small, 11);

	/// Validation failed because drive deposit is already returned.
	DEFINE_SERVICE_RESULT(Drive_Deposit_Already_Returned, 12);

	/// Validation failed because returned drive deposit is invalid.
	DEFINE_SERVICE_RESULT(Returned_Drive_Deposit_Invalid, 13);

	/// Validation failed because returned drive deposit is invalid.
	DEFINE_SERVICE_RESULT(Returned_File_Deposit_Invalid, 14);

	/// Validation failed because file deposit is already returned.
	DEFINE_SERVICE_RESULT(File_Deposit_Already_Returned, 15);

	/// Validation failed because a drive replicator is not registered.
	DEFINE_SERVICE_RESULT(Drive_Replicator_Not_Registered, 16);

	/// Validation failed because a drive directory already exists.
	DEFINE_SERVICE_RESULT(Drive_Directory_Exists, 17);

	/// Validation failed because a drive directory doesn't  exist.
	DEFINE_SERVICE_RESULT(Drive_Directory_Doesnt_Exist, 18);

	/// Validation failed because a drive parent directory doesn't  exist.
	DEFINE_SERVICE_RESULT(Drive_Parent_Directory_Doesnt_Exist, 19);

	/// Validation failed because a drive file already exists.
	DEFINE_SERVICE_RESULT(Drive_File_Exists, 20);

	/// Validation failed because a drive file doesn't exist.
	DEFINE_SERVICE_RESULT(Drive_File_Doesnt_Exist, 21);

	/// Validation failed because destination file and source file are from different drives.
	DEFINE_SERVICE_RESULT(Desitination_And_Source_Are_From_Different_Drives, 22);

	/// Validation failed because destination file and source file have different hash.
	DEFINE_SERVICE_RESULT(Desitination_And_Source_Have_Different_Hash, 23);

	/// Validation failed because not mosaic is not valid.
	DEFINE_SERVICE_RESULT(Invalid_Mosaic, 24);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
