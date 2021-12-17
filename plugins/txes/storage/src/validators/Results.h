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

	/// Not every key in the opinion appears exactly once.
	DEFINE_STORAGE_RESULT(Opinion_Reocurring_Keys, 21);

	/// The key in upload opinion is neither a key of one of the current replicators of the drive nor a key of the drive owner.
	DEFINE_STORAGE_RESULT(Opinion_Invalid_Key, 22);

	/// Percents in replicator's upload opinion do not sum up to 100.
	DEFINE_STORAGE_RESULT(Opinion_Incorrect_Percentage, 23);

	/// Data modification is not present in activeDataModifications.
	DEFINE_STORAGE_RESULT(No_Confirmed_Used_Sizes, 24);

	/// Respective BLS public key already exists in BLS keys cache.
	DEFINE_STORAGE_RESULT(BLS_Key_Already_Registered, 25);

	/// Opinion index is out of range.
	DEFINE_STORAGE_RESULT(Invalid_Opinion_Index, 26);

	/// Signature doesn't match the message (opinion).
	DEFINE_STORAGE_RESULT(Opinion_Invalid_Signature, 27);

	/// The key is present in the list of public keys, but no opinion about it is given.
	DEFINE_STORAGE_RESULT(Opinion_Unused_Key, 28);

	/// Not every individual part of the multisig transaction appears exactly once.
	DEFINE_STORAGE_RESULT(Opinions_Reocurring_Individual_Parts, 29);

	/// Multisig transaction for the corresponding billing period has already been approved.
	DEFINE_STORAGE_RESULT(Transaction_Already_Approved, 30);

	/// Download approval transaction sequence number is invalid
	DEFINE_STORAGE_RESULT(Invalid_Sequence_Number, 31);

	/// Sender's account state is not found.
	DEFINE_STORAGE_RESULT(Sender_State_Not_Found, 32);

	/// Recipient's account state is not found.
	DEFINE_STORAGE_RESULT(Recipient_State_Not_Found, 33);

	/// Replicator hasn't provided an opinion on itself.
	DEFINE_STORAGE_RESULT(No_Opinion_Provided_On_Self, 34);

	/// Opinion is provided, but its index does not appear in the list of opinion indices.
	DEFINE_STORAGE_RESULT(Unused_Opinion, 35);

	/// Replicator has provided an opinion on itself when he shouldn't have done this.
	DEFINE_STORAGE_RESULT(Opinion_Provided_On_Self, 36);

	/// There are no drive infos in the replicator entry with given drive key.
	DEFINE_STORAGE_RESULT(Drive_Info_Not_Found, 37);

	/// Verification Trigger is not equal to the pending verification.
	DEFINE_STORAGE_RESULT(Verification_Bad_Verification_Trigger, 38);

	/// The provided count of Provers is not equal to desired.
	DEFINE_STORAGE_RESULT(Verification_Wrong_Number_Of_Provers, 39);

	/// Not all Provers were in the Confirmed state at the start of Verification.
	DEFINE_STORAGE_RESULT(Verification_Some_Provers_Are_Illegal, 40);

	/// Validation failed because the data modification already exists.
	DEFINE_STORAGE_RESULT(Stream_Already_Exists, 41);

	/// Validation failed because the stream if not first in the queue.
	DEFINE_STORAGE_RESULT(Invalid_Stream_Id, 42);

	/// Validation failed because the stream has already been finished.
	DEFINE_STORAGE_RESULT(Stream_Already_Finished, 43);

	/// Validation failed because declared stream actual size exceeds prepaid expected size
	DEFINE_STORAGE_RESULT(Expected_Upload_Size_Exceeded, 44);

	/// Desired drive size is greater than maximal.
	DEFINE_STORAGE_RESULT(Drive_Size_Excessive, 45);

	/// Desired modification size is greater than maximal.
	DEFINE_STORAGE_RESULT(Upload_Size_Excessive, 46);

	/// Desired download size is greater than maximal.
	DEFINE_STORAGE_RESULT(Download_Size_Excessive, 47);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
