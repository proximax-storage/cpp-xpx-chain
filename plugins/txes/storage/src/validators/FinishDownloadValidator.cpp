/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DownloadChannelCache.h"

namespace catapult { namespace validators {

	using Notification = model::FinishDownloadNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(FinishDownload, [](const model::FinishDownloadNotification<1>& notification, const ValidatorContext& context) {
		const auto& downloadChannelCache = context.Cache.sub<cache::DownloadChannelCache>();
		const auto downloadChannelIter = downloadChannelCache.find(notification.DownloadChannelId);
		const auto& pDownloadChannelEntry = downloadChannelIter.tryGet();

		// Check if download channel exists
		if (!pDownloadChannelEntry)
			return Failure_Storage_Download_Channel_Not_Found;

		if (pDownloadChannelEntry->isCloseInitiated()) {
			return Failure_Storage_Already_Initiated_Channel_Closure;
		}

	  	// Check if transaction signer is the owner of the download channel
	  	if (notification.PublicKey != pDownloadChannelEntry->consumer())
		  	return Failure_Storage_Is_Not_Owner;

		return ValidationResult::Success;
	});

}}
