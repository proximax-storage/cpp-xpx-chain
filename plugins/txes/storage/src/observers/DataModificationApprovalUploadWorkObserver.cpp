/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <boost/dynamic_bitset.hpp>
#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(DataModificationApprovalUploadWork, model::DataModificationApprovalUploadWorkNotification<1>, [](const model::DataModificationApprovalUploadWorkNotification<1>& notification, ObserverContext& context) {
	  	if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DataModificationApprovalUploadWork)");

	  	const auto totalKeysCount = notification.JudgingKeysCount + notification.OverlappingKeysCount + notification.JudgedKeysCount;
	  	const auto totalJudgingKeysCount = totalKeysCount - notification.JudgedKeysCount;
	  	const auto totalJudgedKeysCount = totalKeysCount - notification.JudgingKeysCount;

		auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
	  	auto driveIter = driveCache.find(notification.DriveKey);
	  	auto& driveEntry = driveIter.get();

	  	auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
	  	auto senderIter = accountStateCache.find(notification.DriveKey);
	  	auto& senderState = senderIter.get();

		const auto& streamingMosaicId = context.Config.Immutable.StreamingMosaicId;
	  	const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;

	  	// Preparing presentOpinions bitset array.
	  	const auto presentOpinionByteCount = (totalJudgingKeysCount * totalJudgedKeysCount + 7) / 8;
	  	boost::dynamic_bitset<uint8_t> presentOpinions(notification.PresentOpinionsPtr, notification.PresentOpinionsPtr + presentOpinionByteCount);

		// Preparing vectors related to cumulative upload sizes.
		std::vector<uint64_t> initialCumulativeUploadSizes;
	  	initialCumulativeUploadSizes.reserve(totalJudgedKeysCount);
	  	const auto& driveOwnerPublicKey = driveEntry.owner();
		for (auto i = notification.JudgingKeysCount; i < totalKeysCount; ++i) {
			const auto key = notification.PublicKeysPtr[i];
			const auto initialSize = (key != driveOwnerPublicKey) ?
					driveEntry.cumulativeUploadSizes().at(key) :
					driveEntry.ownerCumulativeUploadSize();
			initialCumulativeUploadSizes.push_back(initialSize);
		}
	  	std::vector<uint64_t> uploadSizesIncrements(totalJudgedKeysCount);

		// Iterating over opinions row by row and calculating upload sizes increments for each judged uploader.
		auto pOpinion = notification.OpinionsPtr;
		for (auto i = 0; i < totalJudgingKeysCount; ++i) {
			for (auto j = 0; j < totalJudgedKeysCount; ++j) {
				if (presentOpinions[i*totalJudgedKeysCount + j]) {
					uploadSizesIncrements.at(j) += std::max(*pOpinion - initialCumulativeUploadSizes.at(j), 0ul);
					++pOpinion;
				}
			}
		}

	  	// Making mosaic transfers and updating cumulative upload sizes.
		auto pKey = &notification.PublicKeysPtr[notification.JudgingKeysCount];
	  	for (auto i = 0; i < totalJudgedKeysCount; ++i, ++pKey) {
			auto recipientIter = accountStateCache.find(*pKey);
			auto& recipientState = recipientIter.get();
			const auto transferAmount = Amount(uploadSizesIncrements.at(i));
			senderState.Balances.debit(streamingMosaicId, transferAmount, context.Height);
			recipientState.Balances.credit(currencyMosaicId, transferAmount, context.Height);
			if (*pKey != driveOwnerPublicKey)
				driveEntry.cumulativeUploadSizes().at(*pKey) += uploadSizesIncrements.at(i);
			else
				driveEntry.increaseOwnerCumulativeUploadSize(uploadSizesIncrements.at(i));
		}
	});
}}
