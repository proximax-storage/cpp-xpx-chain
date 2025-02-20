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

	 	const auto totalJudgingKeysCount = notification.JudgingKeysCount + notification.OverlappingKeysCount;
	  	const auto totalJudgedKeysCount = notification.OverlappingKeysCount + notification.JudgedKeysCount;

		// Check if download channel exists
		if (!pDownloadChannelEntry)
			return Failure_Storage_Download_Channel_Not_Found;

		// Check if transaction approval trigger is the actual one
		if (!pDownloadChannelEntry->downloadApprovalInitiationEvent() ||
		*pDownloadChannelEntry->downloadApprovalInitiationEvent() != notification.ApprovalTrigger) {
			return Failure_Storage_Invalid_Approval_Trigger;
		}

	  	// Check if there are enough cosigners
	  	const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
		const auto& shardReplicators = pDownloadChannelEntry->shardReplicators();
	  	if (totalJudgingKeysCount < (std::max<size_t>(shardReplicators.size(), pluginConfig.MinReplicatorCount) * 2) / 3 + 1)
		  	return Failure_Storage_Signature_Count_Insufficient;

	  	// Check if every replicator has provided an opinion on itself
		if (notification.JudgingKeysCount > 0)
			return Failure_Storage_No_Opinion_Provided_On_Self;
		const auto presentOpinionByteCount = (notification.OverlappingKeysCount * totalJudgedKeysCount + 7) / 8;
	  	boost::dynamic_bitset<uint8_t> presentOpinions(notification.PresentOpinionsPtr, notification.PresentOpinionsPtr + presentOpinionByteCount);
		for (auto i = 0; i < notification.OverlappingKeysCount; ++i)
			if (!presentOpinions[i*totalJudgedKeysCount + i])
				return Failure_Storage_No_Opinion_Provided_On_Self;

	  	// Check if all judging keys belong to the download channel's shard
	  	for (auto i = 0; i < totalJudgingKeysCount; ++i)
		  	if (!shardReplicators.count(notification.PublicKeysPtr[i]))
			  	return Failure_Storage_Opinion_Invalid_Key;

		return ValidationResult::Success;
	});

}}