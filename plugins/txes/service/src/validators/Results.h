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

	/// Drive duration should be multiple of the billing period.
	DEFINE_SERVICE_RESULT(Drive_Duration_Is_Not_Divide_On_BillingPeriod, 1);

	/// Percent approvers is not in range 0 - 100.
	DEFINE_SERVICE_RESULT(Wrong_Percent_Approvers, 2);

	/// Minimal count of replicators to start the contract more than count of replicas.
	DEFINE_SERVICE_RESULT(Min_Replicators_More_Than_Relicas, 3);

    /// Validation failed because drive duration is zero.
    DEFINE_SERVICE_RESULT(Drive_Invalid_Duration, 4);

    /// Validation failed because drive billing period is zero.
    DEFINE_SERVICE_RESULT(Drive_Invalid_Billing_Period, 5);

    /// Validation failed because drive billing price is zero.
    DEFINE_SERVICE_RESULT(Drive_Invalid_Billing_Price, 6);

    /// Validation failed because size is zero.
    DEFINE_SERVICE_RESULT(Drive_Invalid_Size, 7);

    /// Validation failed because count of replicas is zero.
    DEFINE_SERVICE_RESULT(Drive_Invalid_Replicas, 8);

    /// Validation failed because duration is zero.
    DEFINE_SERVICE_RESULT(Drive_Invalid_Min_Replicators, 9);

	/// Validation failed because the drive already exists.
	DEFINE_SERVICE_RESULT(Drive_Alredy_Exists, 10);

	/// Validation failed because plugin configuration data is malformed.
	DEFINE_SERVICE_RESULT(Plugin_Config_Malformed, 11);

	/// Validation failed because operation is not permitted for drive account.
	DEFINE_SERVICE_RESULT(Operation_Is_Not_Permitted, 12);

	/// Validation failed because the drive doesn't exist.
	DEFINE_SERVICE_RESULT(Drive_Does_Not_Exist, 13);

	/// Validation failed because the replicator already connected to drive.
	DEFINE_SERVICE_RESULT(Replicator_Already_Connected_To_Drive, 14);

	/// Validation failed because the root hash of transaction is not equal to root hash of drive in db.
	DEFINE_SERVICE_RESULT(Root_Hash_Is_Not_Equal, 15);

	/// Validation failed because the drive already contains file with the same hash.
	DEFINE_SERVICE_RESULT(File_Hash_Redudant, 16);

	/// Validation failed because the drive doesn't contain file to remove.
	DEFINE_SERVICE_RESULT(File_Is_Not_Exist, 17);

	/// Validation failed because the drive contains to many files.
	DEFINE_SERVICE_RESULT(Too_Many_Files_On_Drive, 18);

	/// Validation failed because a drive replicator is not registered.
	DEFINE_SERVICE_RESULT(Drive_Replicator_Not_Registered, 19);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
