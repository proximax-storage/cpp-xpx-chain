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

	/// Desired number of replicators is less than minimal,
	/// or offboarding of the replicator is not possible as drive's actual replicator count will become less than minimal,
	/// or there are not enough replicators on the drive.
	DEFINE_STORAGE_RESULT(Replicator_Count_Insufficient, 2);

	/// Desired replicator capacity is less than minimal.
	DEFINE_STORAGE_RESULT(Replicator_Capacity_Insufficient, 3);

	/// Validation failed because plugin configuration data is malformed.
	DEFINE_STORAGE_RESULT(Plugin_Config_Malformed, 4);

	/// Validation failed because the drive already exists.
	DEFINE_STORAGE_RESULT(Drive_Already_Exists, 5);

	/// Data modification is not present in activeDataModifications.
	DEFINE_STORAGE_RESULT(No_Active_Data_Modifications, 6);

	/// There are other active data modifications that need to be finished first.
	DEFINE_STORAGE_RESULT(Invalid_Data_Modification_Id, 7);

	/// Validation failed DataModificationTransaction is not found.
	DEFINE_STORAGE_RESULT(Data_Modification_Not_Found, 8);

	/// Validation failed because transaction signer is not an owner of the drive or download channel.
	DEFINE_STORAGE_RESULT(Is_Not_Owner, 9);

	/// Validation failed because drive does not exist.
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

	/// Validation failed because replicator not registered.
	DEFINE_STORAGE_RESULT(Replicator_Not_Registered, 16);

	/// Respective download channel is not found.
	DEFINE_STORAGE_RESULT(Download_Channel_Not_Found, 17);

	/// Signer of the transaction is not allowed to issue transactions of such type.
	DEFINE_STORAGE_RESULT(Invalid_Transaction_Signer, 18);

	/// Respective drive is not assigned to respective replicator.
	DEFINE_STORAGE_RESULT(Drive_Not_Assigned_To_Replicator, 19);

	/// There are no data modifications in completedDataModifications with 'succeeded' state.
	DEFINE_STORAGE_RESULT(No_Approved_Data_Modifications, 20);

	/// Used drive size exceeds total size of the drive.
	DEFINE_STORAGE_RESULT(Invalid_Used_Size, 21);

	/// Not every key in the opinion appears exactly once.
	DEFINE_STORAGE_RESULT(Opinion_Duplicated_Keys, 22);

	/// The key in upload opinion is neither a key of one of the current replicators of the drive nor a key of the drive owner.
	DEFINE_STORAGE_RESULT(Opinion_Invalid_Key, 23);

	/// Incorrect value in opinion.
	DEFINE_STORAGE_RESULT(Invalid_Opinion, 24);

	/// Incorrect sum of opinions in data modification approval transaction.
	DEFINE_STORAGE_RESULT(Invalid_Opinions_Sum, 25);

	/// Data modification is not present in activeDataModifications.
	DEFINE_STORAGE_RESULT(No_Confirmed_Used_Sizes, 26);

	/// Public keys mentioned in the transaction appear in wrong order.
	DEFINE_STORAGE_RESULT(Opinion_Invalid_Key_Order, 27);

	/// Opinion index is out of range.
	DEFINE_STORAGE_RESULT(Opinion_Invalid_Index, 28);

	/// Signature doesn't match the message (opinion).
	DEFINE_STORAGE_RESULT(Opinion_Invalid_Signature, 29);

	/// The key is present in the list of public keys, but no opinion about it is given.
	DEFINE_STORAGE_RESULT(Opinion_Unused_Key, 30);

	/// Not every individual part of the multisig transaction appears exactly once.
	DEFINE_STORAGE_RESULT(Opinions_Reocurring_Individual_Parts, 31);

	/// Multisig transaction for the corresponding billing period has already been approved.
	DEFINE_STORAGE_RESULT(Transaction_Already_Approved, 32);

	/// Download approval transaction sequence number is invalid
	DEFINE_STORAGE_RESULT(Invalid_Approval_Trigger, 33);

	/// Replicator hasn't provided an opinion on itself.
	DEFINE_STORAGE_RESULT(No_Opinion_Provided_On_Self, 34);

	/// Opinion is provided, but its index does not appear in the list of opinion indices.
	DEFINE_STORAGE_RESULT(Unused_Opinion, 35);

	/// Replicator has provided an opinion on itself when he shouldn't have done this.
	DEFINE_STORAGE_RESULT(Opinion_Provided_On_Self, 36);

	/// There are no drive infos in the replicator entry with given drive key.
	DEFINE_STORAGE_RESULT(Drive_Info_Not_Found, 37);

	/// Verification Trigger is not equal to the pending verification.
	DEFINE_STORAGE_RESULT(Bad_Verification_Trigger, 38);

	/// The provided count of Provers is not equal to desired.
	DEFINE_STORAGE_RESULT(Verification_Not_In_Progress, 39);

	/// Not all Provers were in the Confirmed state at the start of Verification.
	DEFINE_STORAGE_RESULT(Verification_Invalid_Prover_Count, 40);

	/// Shard ID exceeds the number of verification shards.
	DEFINE_STORAGE_RESULT(Verification_Invalid_Shard_Id, 41);

	/// Not all Provers were in the Confirmed state at the start of Verification.
	DEFINE_STORAGE_RESULT(Verification_Invalid_Prover, 42);

	/// Validation failed because the data modification already exists.
	DEFINE_STORAGE_RESULT(Stream_Already_Exists, 43);

	/// Validation failed because the stream if not first in the queue.
	DEFINE_STORAGE_RESULT(Invalid_Stream_Id, 44);

	/// Validation failed because the stream has already been finished.
	DEFINE_STORAGE_RESULT(Stream_Already_Finished, 45);

	/// Validation failed because declared stream actual size exceeds prepaid expected size
	DEFINE_STORAGE_RESULT(Expected_Upload_Size_Exceeded, 46);

	/// Desired drive size is greater than maximal.
	DEFINE_STORAGE_RESULT(Drive_Size_Excessive, 47);

	/// Desired modification size is greater than maximal.
	DEFINE_STORAGE_RESULT(Upload_Size_Excessive, 48);

	/// Desired download size is greater than maximal.
	DEFINE_STORAGE_RESULT(Download_Size_Excessive, 49);

	/// Desired download size is less than minimal.
	DEFINE_STORAGE_RESULT(Download_Size_Insufficient, 50);

	/// Number of signatures in opinion-based multisignature transaction is less than minimal.
	DEFINE_STORAGE_RESULT(Signature_Count_Insufficient, 51);

	/// The replicator has already applied for offboarding from the drive.
	DEFINE_STORAGE_RESULT(Already_Applied_For_Offboarding, 52);

	/// The replicator has already applied for offboarding from the drive.
	DEFINE_STORAGE_RESULT(Already_Initiated_Channel_Closure, 53);

	/// Download channels is finished
	DEFINE_STORAGE_RESULT(Download_Channel_Is_Finished, 54);

	/// Attempting to transfer a service unit.
	DEFINE_STORAGE_RESULT(Service_Unit_Transfer, 55);

	/// Validation failed because supercontract is already deployed.
	DEFINE_STORAGE_RESULT(Owner_Management_Is_Forbidden, 56);

	/// Validation failed because too many replicators have been ordered
	DEFINE_STORAGE_RESULT(Replicator_Count_Exceeded, 57);

	/// Modification not ready for approval
	DEFINE_STORAGE_RESULT(Modification_Not_Ready_For_Approval, 58);

	/// Modification upload size is invalid
	DEFINE_STORAGE_RESULT(Modification_Invalid_Upload_Size, 59);

	/// Boot key is already registered with other replicator
	DEFINE_STORAGE_RESULT(Boot_Key_Is_Registered_With_Other_Replicator, 60);

	/// No replicators to remove
	DEFINE_STORAGE_RESULT(No_Replicators_To_Remove, 61);

	/// Replicator is bound with a boot key
	DEFINE_STORAGE_RESULT(Replicator_Is_Bound_With_Boot_Key, 62);

	// Validation failed because there are modifications in progress
	DEFINE_STORAGE_RESULT(Modification_In_Progress, 60);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
