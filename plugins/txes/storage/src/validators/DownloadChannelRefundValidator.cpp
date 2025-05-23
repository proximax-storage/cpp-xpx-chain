/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::DownloadChannelRefundNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(DownloadChannelRefund, ([](const Notification& notification, const ValidatorContext& context) {
  		const auto& downloadChannelCache = context.Cache.sub<cache::DownloadChannelCache>();
	  	const auto downloadChannelIter = downloadChannelCache.find(notification.DownloadChannelId);
	  	const auto& pDownloadChannelEntry = downloadChannelIter.tryGet();

		// Check if download channel exists
		if (!pDownloadChannelEntry)
			return Failure_Storage_Download_Channel_Not_Found;

		return ValidationResult::Success;
	}))

}}
