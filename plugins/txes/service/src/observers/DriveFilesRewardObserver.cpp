/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/DriveCache.h"
#include <cmath>

namespace catapult { namespace observers {

	using Notification = model::DriveFilesRewardNotification<1>;

	DECLARE_OBSERVER(DriveFilesReward, Notification)(const config::ImmutableConfiguration& config) {
		return MAKE_OBSERVER(DriveFilesReward, Notification, [&config](const Notification& notification, ObserverContext& context) {
			auto& driveCache = context.Cache.sub<cache::DriveCache>();
			auto driveIter = driveCache.find(notification.DriveKey);
			auto& driveEntry = driveIter.get();
			auto streamingMosaicId = config.StreamingMosaicId;

			auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
			auto accountIter = accountStateCache.find(driveEntry.key());
			auto& driveAccount = accountIter.get();

			if (NotifyMode::Commit == context.Mode) {
				auto sum = utils::GetDriveBalance(driveEntry, context.Cache, streamingMosaicId);
				uint64_t sumUpload = 0;

				auto iter = notification.UploadInfoPtr;
				for (auto i = 0u; i < notification.UploadInfosCount; ++i, ++iter) {
					sumUpload += iter->Uploaded;
				}

				iter = notification.UploadInfoPtr;
				for (auto i = 0u; i < notification.UploadInfosCount; ++i, ++iter) {
					auto reward = Amount(std::floor(double(sum.unwrap()) * iter->Uploaded / sumUpload));

					// The last participant takes remaining tokens, it is need to resolve the integer division
					if (i == notification.UploadInfosCount - 1)
						reward = utils::GetDriveBalance(driveEntry, context.Cache, streamingMosaicId);

					auto participantIter = accountStateCache.find(iter->Participant);
					auto& participantAccount = participantIter.get();
					Transfer(driveAccount, participantAccount, streamingMosaicId, reward, context);

					driveEntry.uploadPayments().emplace_back(state::PaymentInformation{
						iter->Participant,
						reward,
						context.Height
					});
				}

				// if drive is finished, it is last payment transaction. Now we can remove drive.
				if (driveEntry.state() >= state::DriveState::Finished) {
					driveCache.markRemoveDrive(driveEntry.key(), context.Height);
					driveEntry.setEnd(context.Height);
					RemoveDriveMultisig(driveEntry, context);
				}
			} else {
				if (driveEntry.state() >= state::DriveState::Finished) {
					driveCache.unmarkRemoveDrive(driveEntry.key(), context.Height);
					driveEntry.setEnd(Height(0));
					RemoveDriveMultisig(driveEntry, context);
				}

				auto& payments = driveEntry.uploadPayments();
				auto iter = notification.UploadInfoPtr + notification.UploadInfosCount - 1;
				for (int i = notification.UploadInfosCount - 1; i >= 0; --i, --iter) {
					const auto& payment = payments.back();
					if (payment.Height != context.Height || iter->Participant != payment.Receiver)
						CATAPULT_THROW_RUNTIME_ERROR("rollback drive files reward unexpected state");

					auto participantIter = accountStateCache.find(payment.Receiver);
					auto& participantAccount = participantIter.get();
					Transfer(participantAccount, driveAccount, streamingMosaicId, payment.Amount, context);
					payments.pop_back();
				}
			}
		});
	};
}}
