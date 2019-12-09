/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DriveCache.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::DriveFilesRewardNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(DriveFilesReward, Notification)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return MAKE_STATEFUL_VALIDATOR(DriveFilesReward, [pConfigHolder](const Notification &notification,
																		 const ValidatorContext &context) {
			const auto &driveCache = context.Cache.sub<cache::DriveCache>();
			auto driveIter = driveCache.find(notification.DriveKey);
			const auto &driveEntry = driveIter.get();

			if (notification.UploadInfosCount == 0)
				return Failure_Service_Zero_Infos;

			if (driveEntry.state() != state::DriveState::Pending && driveEntry.state() != state::DriveState::Finished)
				return Failure_Service_Drive_Not_In_Pending_State;

			auto streamingMosaicId = pConfigHolder->Config(context.Height).Immutable.StreamingMosaicId;
			if (utils::GetBalanceOfDrive(driveEntry, context.Cache, streamingMosaicId) == Amount(0))
				return Failure_Service_Doesnt_Contains_Streaming_Tokens;

			auto pInfo = notification.UploadInfoPtr;
			utils::KeySet keys;
			for (auto i = 0u; i < notification.UploadInfosCount; ++i, ++pInfo) {
				keys.insert(pInfo->Participant);

				if (pInfo->Uploaded == 0)
					return Failure_Service_Zero_Upload_Info;

				if (!driveEntry.hasReplicator(pInfo->Participant) && driveEntry.owner() != pInfo->Participant)
					return Failure_Service_Participant_It_Not_Part_Of_Drive;

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
