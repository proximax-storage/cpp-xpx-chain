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
		for (auto i = 0; i < totalJudgingKeysCount; ++i, ++pKey)
			if (*pKey != driveOwnerPublicKey && !pDriveEntry->replicators().count(*pKey))
				return Failure_Storage_Replicator_Not_Found;

		// Check if
		// - each replicator has given an opinion exclusively on the replicators of his shard
		// - all opinions are not less than respective cumulativeUploadSizes
		// - each replicator's opinion increments sum up to his LastCompletedCumulativeDownloadWork
	  	const auto presentOpinionByteCount = (totalJudgingKeysCount * totalJudgedKeysCount + 7) / 8;
	  	boost::dynamic_bitset<uint8_t> presentOpinions(notification.PresentOpinionsPtr, notification.PresentOpinionsPtr + presentOpinionByteCount);

		uint64_t unaccountedSizeBytes = 0;
		if (!pDriveEntry->activeDataModifications().empty()
		&& pDriveEntry->activeDataModifications().front().Id == notification.ModificationId) {
			unaccountedSizeBytes += utils::FileSize::FromMegabytes(pDriveEntry->activeDataModifications().front().ActualUploadSizeMegabytes).bytes();
		}

		const auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
		auto pOpinion = notification.OpinionsPtr;
		for (auto i = 0; i < totalJudgingKeysCount; ++i) {
			const auto judgingKey = notification.PublicKeysPtr[i];
			const auto& shardsPair = pDriveEntry->dataModificationShards().at(judgingKey);
			uint64_t totalCumulativeUpload = 0;
			for (auto j = 0; j < totalJudgedKeysCount; ++j) {
				if (presentOpinions[i*totalJudgedKeysCount + j]) {
					const auto judgedKey = notification.PublicKeysPtr[notification.JudgingKeysCount + j];

					uint64_t initialCumulativeUploadSize;
					if ( auto it = shardsPair.ActualShardMembers.find(judgedKey); it != shardsPair.ActualShardMembers.end() ) {
						initialCumulativeUploadSize = it->second;
					}
					else if (auto it = shardsPair.FormerShardMembers.find(judgedKey); it != shardsPair.FormerShardMembers.end()) {
						initialCumulativeUploadSize = it->second;
					}
					else if (judgedKey == driveOwnerPublicKey) {
						initialCumulativeUploadSize = shardsPair.OwnerUpload;
					}
					else {
						return Failure_Storage_Opinion_Invalid_Key;
					}

					if (*pOpinion < initialCumulativeUploadSize) {
						return Failure_Storage_Invalid_Opinion;
					}
					const auto increment = *pOpinion - initialCumulativeUploadSize;

					auto totalCumulativeUploadTemp = totalCumulativeUpload + *pOpinion++;

					if (totalCumulativeUploadTemp < totalCumulativeUpload) {
						// Overflow detected
						// TODO Use safemath
						return Failure_Storage_Invalid_Opinion;
					}

					totalCumulativeUpload = totalCumulativeUploadTemp;
				}
			}
			const auto replicatorIter = replicatorCache.find(judgingKey);
			const auto& pReplicatorEntry = replicatorIter.get();

			auto expectedTotalCumulativeUpload = pReplicatorEntry.drives().at(notification.DriveKey).LastCompletedCumulativeDownloadWorkBytes + unaccountedSizeBytes;

			if (expectedTotalCumulativeUpload != totalCumulativeUpload) {
				return Failure_Storage_Invalid_Opinions_Sum;
			}
		}

	  	// TODO: Check if there are enough mosaics for the transfer?

		return ValidationResult::Success;
	});
}}
