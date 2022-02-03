/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include <boost/dynamic_bitset.hpp>
#include "src/cache/BcDriveCache.h"

namespace catapult { namespace validators {

	using Notification = model::DataModificationApprovalUploadWorkNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(DataModificationApprovalUploadWork, [](const Notification& notification, const ValidatorContext& context) {
	  	const auto totalKeysCount = notification.JudgingKeysCount + notification.OverlappingKeysCount + notification.JudgedKeysCount;
	  	const auto totalJudgingKeysCount = totalKeysCount - notification.JudgedKeysCount;
	  	const auto totalJudgedKeysCount = totalKeysCount - notification.JudgingKeysCount;

		const auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
	  	const auto driveIter = driveCache.find(notification.DriveKey);
	 	const auto& pDriveEntry = driveIter.tryGet();

	  	// Check if respective drive exists
	  	if (!pDriveEntry)
		  	return Failure_Storage_Drive_Not_Found;

		// Check if all replicators' keys are present in drive's cumulativeUploadSizes
		const auto& driveOwnerPublicKey = pDriveEntry->owner();
		auto pKey = &notification.PublicKeysPtr[notification.JudgingKeysCount];
		for (auto i = 0; i < totalJudgedKeysCount; ++i, ++pKey)
			if (*pKey != driveOwnerPublicKey && !pDriveEntry->cumulativeUploadSizes().count(*pKey))
				return Failure_Storage_Replicator_Not_Found;

		// Check if
		// - each replicator has given an opinion exclusively on the replicators of his shard
		// - all opinions are not less than respective cumulativeUploadSizes
		// - each replicator's opinion increments sum up to his LastCompletedCumulativeDownloadWork
	  	const auto presentOpinionByteCount = (totalJudgingKeysCount * totalJudgedKeysCount + 7) / 8;
	  	boost::dynamic_bitset<uint8_t> presentOpinions(notification.PresentOpinionsPtr, notification.PresentOpinionsPtr + presentOpinionByteCount);

	  	std::vector<uint64_t> initialCumulativeUploadSizes;
	  	initialCumulativeUploadSizes.reserve(totalJudgedKeysCount);
	  	for (auto i = notification.JudgingKeysCount; i < totalKeysCount; ++i) {
		  	const auto key = notification.PublicKeysPtr[i];
		  	const auto initialSize = (key != driveOwnerPublicKey) ?
					pDriveEntry->cumulativeUploadSizes().at(key) :
					pDriveEntry->ownerCumulativeUploadSize();
		  	initialCumulativeUploadSizes.push_back(initialSize);
	  	}

		const auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
		auto pOpinion = notification.OpinionsPtr;
		for (auto i = 0; i < totalJudgingKeysCount; ++i) {
			const auto judgingKey = notification.PublicKeysPtr[i];
			const auto& shardsPair = pDriveEntry->dataModificationShards().at(judgingKey);
			uint64_t totalIncrements = 0;
			for (auto j = 0; j < totalJudgedKeysCount; ++j) {
				if (presentOpinions[i*totalJudgedKeysCount + j]) {
					const auto judgedKey = notification.PublicKeysPtr[notification.JudgingKeysCount + j];
					if (!shardsPair.first.count(judgedKey) && !shardsPair.second.count(judgedKey))
						return Failure_Storage_Opinion_Invalid_Key;
					const auto increment = *pOpinion++ - initialCumulativeUploadSizes.at(j);
					if (increment < 0)
						return Failure_Storage_Invalid_Opinion;
					totalIncrements += increment;
				}
			}
			const auto replicatorIter = replicatorCache.find(notification.PublicKeysPtr[i]);
			const auto& pReplicatorEntry = replicatorIter.tryGet();
			if (pReplicatorEntry->drives().at(notification.DriveKey).LastCompletedCumulativeDownloadWork != totalIncrements)
				return Failure_Storage_Invalid_Opinions_Sum;
		}

	  	// TODO: Check if there are enough mosaics for the transfer?

		return ValidationResult::Success;
	});
}}
