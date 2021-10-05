/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DownloadChannelCache.h"

namespace catapult { namespace validators {

	using Notification = model::DownloadApprovalNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(DownloadApproval, [](const Notification& notification, const ValidatorContext& context) {
		const auto& downloadChannelCache = context.Cache.sub<cache::DownloadChannelCache>();
		const auto downloadChannelIter = downloadChannelCache.find(notification.DownloadChannelId);
		const auto& pDownloadChannelEntry = downloadChannelIter.tryGet();

		// Check if download channel exists
		if (!pDownloadChannelEntry)
			return Failure_Storage_Download_Channel_Not_Found;

	  	// Check if transaction sequence number is exactly one more than the number of completed download approval transactions
	  	const auto targetSequenceNumber = pDownloadChannelEntry->downloadApprovalCount() + 1;
		if (notification.SequenceNumber == targetSequenceNumber - 1)		// Considered a regular case
			return Failure_Storage_Transaction_Already_Approved;
		else if (notification.SequenceNumber != targetSequenceNumber)	// Considered an irregular case, may be worth investigating
			return Failure_Storage_Invalid_Sequence_Number;

		return ValidationResult::Success;
	});

}}
