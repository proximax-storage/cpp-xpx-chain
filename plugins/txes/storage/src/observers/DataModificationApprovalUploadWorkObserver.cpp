/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <boost/dynamic_bitset.hpp>
#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER_WITH_LIQUIDITY_PROVIDER(DataModificationApprovalUploadWork, model::DataModificationApprovalUploadWorkNotification<1>, [&liquidityProvider](const model::DataModificationApprovalUploadWorkNotification<1>& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DataModificationApprovalUploadWork)");

		const auto totalKeysCount = notification.JudgingKeysCount + notification.OverlappingKeysCount +
									notification.JudgedKeysCount;
		const auto totalJudgingKeysCount = totalKeysCount - notification.JudgedKeysCount;
		const auto totalJudgedKeysCount = totalKeysCount - notification.JudgingKeysCount;

		auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		auto driveIter = driveCache.find(notification.DriveKey);
		auto& driveEntry = driveIter.get();

		const auto& streamingMosaicId = context.Config.Immutable.StreamingMosaicId;

		// Preparing presentOpinions bitset array.
		const auto presentOpinionByteCount = (totalJudgingKeysCount * totalJudgedKeysCount + 7) / 8;
		boost::dynamic_bitset<uint8_t> presentOpinions(
				notification.PresentOpinionsPtr, notification.PresentOpinionsPtr + presentOpinionByteCount);

		// Preparing vectors related to cumulative upload sizes.
		//		std::vector<uint64_t> initialCumulativeUploadSizes;
		//	  	initialCumulativeUploadSizes.reserve(totalJudgedKeysCount);
		const auto& driveOwnerPublicKey = driveEntry.owner();
		//		for (auto i = notification.JudgingKeysCount; i < totalKeysCount; ++i) {
		//			const auto& key = notification.PublicKeysPtr[i];
		//			const auto& initialSize = (key != driveOwnerPublicKey) ?
		//driveEntry.cumulativeUploadSizesBytes()[key] : driveEntry.ownerCumulativeUploadSizeBytes();
		//			initialCumulativeUploadSizes.push_back(initialSize);
		//		}
		std::vector<uint64_t> uploadSizesIncrements(totalJudgedKeysCount);

		// Iterating over opinions row by row and calculating upload sizes increments for each judged uploader;
		// resetting the set of additional keys in dataModificationShards for each judging replicator.
		auto pOpinion = notification.OpinionsPtr;
		for (auto i = 0; i < totalJudgingKeysCount; ++i) {
			auto& shardsInfo = driveEntry.dataModificationShards().at(notification.PublicKeysPtr[i]);
			for (auto j = 0; j < totalJudgedKeysCount; ++j) {
				if (presentOpinions[i * totalJudgedKeysCount + j]) {
					const auto judgedKey = notification.PublicKeysPtr[notification.JudgingKeysCount + j];

					uint64_t* initialCumulativeUploadSize = nullptr;
					if (auto it = shardsInfo.m_actualShardMembers.find(judgedKey);
						it != shardsInfo.m_actualShardMembers.end()) {
						initialCumulativeUploadSize = &it->second;
					} else if (auto it = shardsInfo.m_formerShardMembers.find(judgedKey);
							   it != shardsInfo.m_formerShardMembers.end()) {
						initialCumulativeUploadSize = &it->second;
					} else {
						initialCumulativeUploadSize = &shardsInfo.m_ownerUpload;
					}

					uploadSizesIncrements.at(j) += *pOpinion - *initialCumulativeUploadSize;
					*initialCumulativeUploadSize = *pOpinion;
					++pOpinion;

					const auto mosaicAmount =
							Amount(utils::FileSize::FromBytes(uploadSizesIncrements.at(i)).megabytes());
					liquidityProvider.debitMosaics(context, driveEntry.key(), judgedKey,
												   config::GetUnresolvedStreamingMosaicId(context.Config.Immutable), mosaicAmount);
				}
			}
		}
	});
}} // namespace catapult::observers