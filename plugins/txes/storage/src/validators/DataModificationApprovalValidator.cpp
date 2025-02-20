/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <boost/dynamic_bitset.hpp>
#include "Validators.h"
#include "src/cache/BcDriveCache.h"

namespace catapult { namespace validators {

	using Notification = model::DataModificationApprovalNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(DataModificationApproval, [](const Notification& notification, const ValidatorContext& context) {
		const auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		const auto driveIter = driveCache.find(notification.DriveKey);
		const auto& pDriveEntry = driveIter.tryGet();

		// Check if respective drive exists
		if (!pDriveEntry)
			return Failure_Storage_Drive_Not_Found;

	  	// Check if new used drive size does not exceed total drive size
//	  	if (notification.UsedDriveSize > pDriveEntry->size())
//		  	return Failure_Storage_Invalid_Used_Size;

	  	// Check if there are any active data modifications
	  	const auto& activeDataModifications = pDriveEntry->activeDataModifications();
		if (activeDataModifications.empty())
			return Failure_Storage_No_Active_Data_Modifications;

		if (!activeDataModifications.begin()->ReadyForApproval)
			return Failure_Storage_Modification_Not_Ready_For_Approval;

	  	// Check if respective data modification is the first (oldest) element in activeDataModifications
	  	if (activeDataModifications.begin()->Id != notification.DataModificationId)
		  	return Failure_Storage_Invalid_Data_Modification_Id;

		const auto& replicators = pDriveEntry->replicators();
	  	const auto& driveOwner = pDriveEntry->owner();

	  	const auto totalJudgingKeysCount = notification.JudgingKeysCount + notification.OverlappingKeysCount;
	  	const auto totalJudgedKeysCount = notification.OverlappingKeysCount + notification.JudgedKeysCount;

	  	// Check if all cosigners keys are replicators
		auto pKey = notification.PublicKeysPtr;
		for (auto i = 0; i < totalJudgingKeysCount; ++i, ++pKey)
	  		if (!replicators.count(*pKey) && *pKey != driveOwner)
	  			return Failure_Storage_Opinion_Invalid_Key;

	  	// Check if there are enough cosigners
		const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
		if (totalJudgingKeysCount < (std::max<size_t>(pDriveEntry->replicators().size(), pluginConfig.MinReplicatorCount) * 2) / 3 + 1)
	  		return Failure_Storage_Signature_Count_Insufficient;

	  	// Check if none of the replicators has provided an opinion on itself
	  	const auto presentOpinionByteCount = (totalJudgingKeysCount * totalJudgedKeysCount + 7) / 8;
	  	boost::dynamic_bitset<uint8_t> presentOpinions(notification.PresentOpinionsPtr, notification.PresentOpinionsPtr + presentOpinionByteCount);
	  	for (auto i = notification.JudgingKeysCount; i < totalJudgingKeysCount; ++i)
			if (presentOpinions[i*totalJudgedKeysCount + (i - notification.JudgingKeysCount)])
				return Failure_Storage_Opinion_Provided_On_Self;

		return ValidationResult::Success;
	});

}}
