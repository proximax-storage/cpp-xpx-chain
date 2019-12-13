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
	DEFINE_SERVICE_RESULT(Drive_Duration_Is_Not_Multiple_Of_BillingPeriod, 1);

	/// Percent approvers is not in range 0 - 100.
	DEFINE_SERVICE_RESULT(Wrong_Percent_Approvers, 2);

	/// Minimal count of replicators to start the contract more than count of replicas.
	DEFINE_SERVICE_RESULT(Min_Replicators_More_Than_Replicas, 3);

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
	DEFINE_SERVICE_RESULT(Drive_Already_Exists, 10);

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
	DEFINE_SERVICE_RESULT(File_Hash_Redundant, 16);

	/// Validation failed because the drive doesn't contain file to remove.
	DEFINE_SERVICE_RESULT(File_Doesnt_Exist, 17);

	/// Validation failed because the drive contains to many files.
	DEFINE_SERVICE_RESULT(Too_Many_Files_On_Drive, 18);

	/// Validation failed because a drive replicator is not registered.
	DEFINE_SERVICE_RESULT(Drive_Replicator_Not_Registered, 19);

	/// Validation failed because drive files system transaction doesn't contain changes.
	DEFINE_SERVICE_RESULT(Drive_Root_No_Changes, 20);

    /// Validation failed because the drive already is finished.
    DEFINE_SERVICE_RESULT(Drive_Has_Ended, 21);

    /// Validation failed because default exchange offer for Xpx to SO units is not exist.
    DEFINE_SERVICE_RESULT(Drive_Cant_Find_Default_Exchange_Offer, 22);

    /// Validation failed because default exchange of this mosaic is not allowed(only allowed SO units).
    DEFINE_SERVICE_RESULT(Exchange_Of_This_Mosaic_Is_Not_Allowed, 23);

    /// Validation failed because drive is not in pending state.
    DEFINE_SERVICE_RESULT(Drive_Not_In_Pending_State, 24);

    /// Validation failed because exchange more SO units than required by drive.
    DEFINE_SERVICE_RESULT(Exchange_More_Than_Required, 25);

    /// Validation failed because exchange cost of SO units is worse than default cost.
    DEFINE_SERVICE_RESULT(Exchange_Cost_Is_Worse_Than_Default, 26);

    /// Validation failed because drive processed full duration.
    DEFINE_SERVICE_RESULT(Drive_Processed_Full_Duration, 27);

    /// Validation failed because upload info is zero.
    DEFINE_SERVICE_RESULT(Zero_Upload_Info, 28);

	/// Validation failed because the participant redundant.
	DEFINE_SERVICE_RESULT(Participant_Redundant, 29);

	/// Validation failed because the participant is not part of drive.
	DEFINE_SERVICE_RESULT(Participant_Is_Not_Registered_To_Drive, 30);

	/// Validation failed because zero deleted files.
	DEFINE_SERVICE_RESULT(Zero_Deleted_Files, 32);

	/// Validation failed because zero infos.
	DEFINE_SERVICE_RESULT(Zero_Infos, 33);

	/// Validation failed because file's deposit is zero.
	DEFINE_SERVICE_RESULT(File_Deposit_Is_Zero, 34);

    /// Validation failed because verification is already in progress.
    DEFINE_SERVICE_RESULT(Verification_Already_In_Progress, 35);

    /// Validation failed because verification has not started.
    DEFINE_SERVICE_RESULT(Verification_Has_Not_Started, 36);

    /// Validation failed because verification is not active.
    DEFINE_SERVICE_RESULT(Verification_Is_Not_Active, 37);

	/// Validation failed because verification has not timed out.
	DEFINE_SERVICE_RESULT(Verification_Has_Not_Timed_Out, 38);

    /// Validation failed because drive is not in progress state.
    DEFINE_SERVICE_RESULT(Drive_Is_Not_In_Progress, 39);

	/// Validation failed because the replicator has active file without deposit.
	DEFINE_SERVICE_RESULT(Replicator_Has_Active_File_Without_Deposit, 40);

	/// Validation failed because the remove file not same file size.
	DEFINE_SERVICE_RESULT(Remove_Files_Not_Same_File_Size, 41);

	/// Validation failed because the drive account doesn't contains streaming tokens.
	DEFINE_SERVICE_RESULT(Doesnt_Contain_Streaming_Tokens, 42);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
