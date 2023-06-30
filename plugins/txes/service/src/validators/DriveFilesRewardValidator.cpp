/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DriveCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/AccountStateCacheUtils.h"
namespace catapult { namespace validators {

	using Notification = model::DriveFilesRewardNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(DriveFilesReward, Notification)(const MosaicId& streamingMosaicId) {
		return MAKE_STATEFUL_VALIDATOR(DriveFilesReward, [streamingMosaicId](const Notification &notification, const ValidatorContext &context) {
			const auto &driveCache = context.Cache.sub<cache::DriveCache>();
			const auto& accountStateCache = context.Cache.template sub<cache::AccountStateCache>();

			auto driveIter = driveCache.find(notification.DriveKey);
			const auto &driveEntry = driveIter.get();

			if (notification.UploadInfosCount == 0)
				return Failure_Service_Zero_Infos;

			if (driveEntry.state() != state::DriveState::Pending && driveEntry.state() != state::DriveState::Finished)
				return Failure_Service_Drive_Not_In_Pending_State;

			if (utils::GetDriveBalance(driveEntry, context.Cache, streamingMosaicId) == Amount(0))
				return Failure_Service_Doesnt_Contain_Streaming_Tokens;

			auto pInfo = notification.UploadInfoPtr;
			utils::KeySet keys;
			for (auto i = 0u; i < notification.UploadInfosCount; ++i, ++pInfo) {
				keys.insert(pInfo->Participant);

				if (pInfo->Uploaded == 0)
					return Failure_Service_Zero_Upload_Info;

				if (!driveEntry.hasReplicator(pInfo->Participant) && cache::GetCurrentlyActiveAccountKey(accountStateCache, driveEntry.owner()) != pInfo->Participant)
					return Failure_Service_Participant_Is_Not_Registered_To_Drive;

				if (driveEntry.hasReplicator(pInfo->Participant)) {
					if (driveEntry.replicators().at(pInfo->Participant).ActiveFilesWithoutDeposit.size() > 0)
						return Failure_Service_Replicator_Has_Active_File_Without_Deposit;
				}
			}

			if (keys.size() != notification.UploadInfosCount)
				return Failure_Service_Participant_Redundant;

			return ValidationResult::Success;
		});
	};
}}
