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

		const auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
	  	const auto senderStateIter = accountStateCache.find(Key(notification.DownloadChannelId.array()));
	  	const auto& pSenderState = senderStateIter.tryGet();

	  	// Check if account state of the download channel exists
		if (!pSenderState)
		  	return Failure_Storage_Sender_State_Not_Found;

		const auto recipientStateIter = accountStateCache.find(pDownloadChannelEntry->consumer());
		const auto& pRecipientState = recipientStateIter.tryGet();

		// Check if account state of the download channel consumer exists
		if (!pRecipientState)
			return Failure_Storage_Recipient_State_Not_Found;

		return ValidationResult::Success;
	}))

}}
