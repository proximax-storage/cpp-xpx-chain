/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <boost/dynamic_bitset.hpp>
#include "Validators.h"
#include "src/cache/DownloadChannelCache.h"

namespace catapult { namespace validators {

	using Notification = model::DownloadApprovalNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(DownloadApproval, [](const Notification& notification, const ValidatorContext& context) {
		const auto& downloadChannelCache = context.Cache.sub<cache::DownloadChannelCache>();
		const auto downloadChannelIter = downloadChannelCache.find(notification.DownloadChannelId);
		const auto& pDownloadChannelEntry = downloadChannelIter.tryGet();

	 	const auto totalKeysCount = notification.JudgingKeysCount + notification.OverlappingKeysCount + notification.JudgedKeysCount;
	  	const auto totalJudgingKeysCount = totalKeysCount - notification.JudgedKeysCount;
	  	const auto totalJudgedKeysCount = totalKeysCount - notification.JudgingKeysCount;

		// Check if download channel exists
		if (!pDownloadChannelEntry)
			return Failure_Storage_Download_Channel_Not_Found;

	  	// Check if there are enough cosigners
	  	if (totalJudgingKeysCount < pDownloadChannelEntry->cumulativePayments().size()*2 / 3 + 1)
		  	return Failure_Storage_Signature_Count_Insufficient;

	  	// Check if transaction sequence number is exactly one more than the number of completed download approval transactions
	  	const auto targetSequenceNumber = pDownloadChannelEntry->downloadApprovalCount() + 1;
		if (notification.SequenceNumber == targetSequenceNumber - 1)		// Considered a regular case
			return Failure_Storage_Transaction_Already_Approved;
		else if (notification.SequenceNumber != targetSequenceNumber)	// Considered an irregular case, may be worth investigating
			return Failure_Storage_Invalid_Sequence_Number;

	  	// Check if every replicator has provided an opinion on itself
		if (notification.JudgingKeysCount > 0)
			return Failure_Storage_No_Opinion_Provided_On_Self;
		const auto presentOpinionByteCount = (notification.OverlappingKeysCount * totalJudgedKeysCount + 7) / 8;
	  	boost::dynamic_bitset<uint8_t> presentOpinions(notification.PresentOpinionsPtr, notification.PresentOpinionsPtr + presentOpinionByteCount);
		for (auto i = 0; i < notification.OverlappingKeysCount; ++i)
			if (!presentOpinions[i*totalJudgedKeysCount + i])
				return Failure_Storage_No_Opinion_Provided_On_Self;

	  	// Check if all public keys belong to the download channel's shard (i.e. they exist in cumulativePayments)
	  	const auto& cumulativePayments = pDownloadChannelEntry->cumulativePayments();
	  	for (auto i = 0; i < totalKeysCount; ++i)
		  	if (!cumulativePayments.count(notification.PublicKeysPtr[i]))
			  	return Failure_Storage_Opinion_Invalid_Key;

		return ValidationResult::Success;
	});

}}
