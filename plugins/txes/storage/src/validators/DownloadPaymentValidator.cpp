/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DownloadChannelCache.h"

namespace catapult { namespace validators {

	using Notification = model::DownloadPaymentNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(DownloadPayment, [](const model::DownloadPaymentNotification<1>& notification, const ValidatorContext& context) {
		const auto& downloadChannelCache = context.Cache.sub<cache::DownloadChannelCache>();
		const auto downloadChannelIter = downloadChannelCache.find(notification.DownloadChannelId);
		const auto& pDownloadChannelEntry = downloadChannelIter.tryGet();

		const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();

		// Check if download channel exists
		if (!pDownloadChannelEntry)
			return Failure_Storage_Download_Channel_Not_Found;

        // Check if download channel is closing
        if (pDownloadChannelEntry->isCloseInitiated())
			return Failure_Storage_Download_Channel_Is_Finished;

		// Check if stream size >= maxDownloadSize
		if (utils::FileSize::FromMegabytes(notification.DownloadSizeMegabytes) > pluginConfig.MaxDownloadSize)
			return Failure_Storage_Upload_Size_Excessive;

		// Check if download channel exists
		if (!pDownloadChannelEntry)
			return Failure_Storage_Download_Channel_Not_Found;

		return ValidationResult::Success;
	});

}}
