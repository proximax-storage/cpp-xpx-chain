/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#ifndef CUSTOM_RESULT_DEFINITION
#include "catapult/validators/ValidationResult.h"

namespace catapult { namespace validators {

#endif
/// Defines a storage validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_STORAGE_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, Storage, DESCRIPTION, CODE, None)

	/// Desired drive size is less than minimal.
	DEFINE_STORAGE_RESULT(Drive_Size_Insufficient, 1);

	/// Desired number of replicators is less than minimal.
	DEFINE_STORAGE_RESULT(Replicator_Count_Insufficient, 2);

	/// Validation failed because plugin configuration data is malformed.
	DEFINE_STORAGE_RESULT(Plugin_Config_Malformed, 3);

	/// Validation failed because the drive already exists.
	DEFINE_STORAGE_RESULT(Drive_Already_Exists, 4);

	/// Data modification is not present in activeDataModifications.
	DEFINE_STORAGE_RESULT(No_Active_Data_Modifications, 5);

	/// There are other active data modifications that need to be finished first.
	DEFINE_STORAGE_RESULT(Invalid_Data_Modification_Id, 6);

	/// Validation failed DataModificationTransaction is not found.
	DEFINE_STORAGE_RESULT(Data_Modification_Not_Found, 7);

	/// Validation failed DataModificationTransaction is Active.
	DEFINE_STORAGE_RESULT(Data_Modification_Is_Active, 8);

	/// Validation failed Transaction Signer is not Drive owner.	// TODO: Can be used for download channel ownership validation?
	DEFINE_STORAGE_RESULT(Is_Not_Owner, 9);

	/// Validation failed becaouse drive does not exist.
	DEFINE_STORAGE_RESULT(Drive_Not_Found, 10);

	/// Validation failed because the data modification already exists.
	DEFINE_STORAGE_RESULT(Data_Modification_Already_Exists, 11);

	/// Validation failed because no replicator registered.
	DEFINE_STORAGE_RESULT(No_Replicator, 12);

	/// Validation failed because no replicator registered.
	DEFINE_STORAGE_RESULT(Multiple_Replicators, 13);

	/// Validation failed because no replicator registered.
	DEFINE_STORAGE_RESULT(Replicator_Not_Found, 14);

	/// Validation failed because no replicator registered.
	DEFINE_STORAGE_RESULT(Replicator_Already_Registered, 15);

	/// Respective download channel is not found.
	DEFINE_STORAGE_RESULT(Download_Channel_Not_Found, 16);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
