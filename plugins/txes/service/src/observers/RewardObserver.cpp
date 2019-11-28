/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/DriveCache.h"
#include "catapult/observers/ObserverUtils.h"
#include "CommonDrive.h"
#include <cmath>

namespace catapult { namespace observers {

	using Notification = model::RewardNotification<1>;

	DECLARE_OBSERVER(Reward, Notification)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return MAKE_OBSERVER(Reward, Notification, [pConfigHolder](const Notification& notification, ObserverContext& context) {
			auto& driveCache = context.Cache.sub<cache::DriveCache>();
			auto driveIter = driveCache.find(notification.DriveKey);
			auto& driveEntry = driveIter.get();
			auto streamingMosaicId = pConfigHolder->Config(context.Height).Immutable.StreamingMosaicId;
			const auto& deletedFile = notification.DeletedFile;
			const auto& fileHash = deletedFile->FileHash;

			auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
			auto accountIter = accountStateCache.find(driveEntry.key());
			auto& driveAccount = accountIter.get();

			state::FileInfo& fileInfo = driveEntry.files().at(fileHash);

			if (NotifyMode::Commit == context.Mode) {
				auto sum = fileInfo.Deposit;
				uint64_t sumUpload = 0;

				auto countActiveReplicators = 0u;
				auto iter = deletedFile->InfosPtr();
				for (auto i = 0u; i < deletedFile->InfosCount(); ++i, ++iter) {
					if (iter->Participant != driveEntry.owner() && driveEntry.replicators().at(iter->Participant).FilesWithoutDeposit.count(fileHash)) {
						continue;
					}
					sumUpload += iter->Uploaded;
					++countActiveReplicators;
				}

				iter = deletedFile->InfosPtr();
				for (auto i = 0u; i < deletedFile->InfosCount(); ++i, ++iter) {
					if (iter->Participant != driveEntry.owner() && driveEntry.replicators().at(iter->Participant).FilesWithoutDeposit.count(fileHash)) {
						continue;
					}
					--countActiveReplicators;
					uint64_t uploaded = iter->Uploaded;
					auto reward = Amount(std::floor(double(sum.unwrap()) * uploaded / sumUpload));

					// The last participant takes remaining tokens, it is need to resolve the integer division
					if (countActiveReplicators == 0)
						reward = fileInfo.Deposit;
					fileInfo.Deposit = fileInfo.Deposit - Amount(reward);

					auto participantIter = accountStateCache.find(iter->Participant);
					auto& participantAccount = participantIter.get();
					Transfer(driveAccount, participantAccount, streamingMosaicId, reward, context.Height);

					fileInfo.Payments.emplace_back(state::PaymentInformation{
						iter->Participant,
						reward,
						context.Height
					});
				}
				driveCache.markRemoveDrive(driveEntry.key(), context.Height);
			} else {
				auto& payments = fileInfo.Payments;
				auto iter = deletedFile->InfosPtr() + deletedFile->InfosCount() - 1;
				for (auto i = deletedFile->InfosCount() - 1; i >= 0 && !payments.empty(); --i, --iter) {
					const auto& payment = payments.back();
					if (payment.Height == context.Height && iter->Participant == payment.Receiver) {
						auto participantIter = accountStateCache.find(payment.Receiver);
						auto& participantAccount = participantIter.get();
						fileInfo.Deposit = fileInfo.Deposit + payment.Amount;
						Transfer(participantAccount, driveAccount, streamingMosaicId, payment.Amount, context.Height);
						payments.erase(--payments.end());
					}
				}
				driveCache.unmarkRemoveDrive(driveEntry.key(), context.Height);
			}
		});
	};
}}
