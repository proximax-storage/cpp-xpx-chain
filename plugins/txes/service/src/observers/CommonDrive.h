/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "Observers.h"
#include "src/cache/DownloadCache.h"
#include "src/cache/DriveCache.h"
#include "src/utils/ServiceUtils.h"
#include "catapult/cache_core/AccountStateCache.h"

namespace catapult { namespace observers {

    void Transfer(state::AccountState& debitState, state::AccountState& creditState, MosaicId mosaicId, Amount amount, ObserverContext& context);

    void Credit(state::AccountState& creditState, MosaicId mosaicId, Amount amount, ObserverContext& context);

    void Debit(state::AccountState& debitState, MosaicId mosaicId, Amount amount, ObserverContext& context);

    void DrivePayment(state::DriveEntry& driveEntry, ObserverContext& context, const MosaicId& storageMosaicId, std::vector<Key> replicators);

	void SetDriveState(state::DriveEntry& entry, ObserverContext& context, state::DriveState driveState);

	void UpdateDriveMultisigSettings(state::DriveEntry& entry, ObserverContext& context);

	void RemoveDriveMultisig(state::DriveEntry& driveEntry, observers::ObserverContext& context);

	template<typename TNotification, NotifyMode AddFileMode>
	void ObserveDownloadNotification(const TNotification& notification, ObserverContext& context) {
		auto& downloadCache = context.Cache.sub<cache::DownloadCache>();
		if (!downloadCache.contains(notification.DriveKey))
			downloadCache.insert(state::DownloadEntry(notification.DriveKey));
		auto downloadIter = downloadCache.find(notification.DriveKey);
		auto& downloadEntry = downloadIter.get();

		if (AddFileMode == context.Mode) {
			auto& downloads = downloadEntry.fileRecipients()[notification.FileRecipient];
			auto& fileHashes = downloads[notification.OperationToken];
			auto pFile = notification.FilesPtr;
			for (auto i = 0; i < notification.FileCount; ++i, ++pFile)
				fileHashes.emplace(pFile->FileHash);
		} else {
			auto& fileRecipients = downloadEntry.fileRecipients();
			auto& downloads = fileRecipients.at(notification.FileRecipient);
			auto& fileHashes = downloads.at(notification.OperationToken);
			auto pFile = notification.FilesPtr;
			for (auto i = 0; i < notification.FileCount; ++i, ++pFile)
				fileHashes.erase(pFile->FileHash);
			if (fileHashes.empty())
				downloads.erase(notification.OperationToken);
			if (downloads.empty())
				fileRecipients.erase(notification.FileRecipient);
			if (fileRecipients.empty())
				downloadCache.remove(notification.DriveKey);
		}
	}

}}
