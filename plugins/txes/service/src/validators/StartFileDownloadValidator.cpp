/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DownloadCache.h"
#include "src/cache/DriveCache.h"

namespace catapult { namespace validators {

	using Notification = model::StartFileDownloadNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(StartFileDownload, [](const Notification &notification, const ValidatorContext &context) {
		const auto& driveCache = context.Cache.sub<cache::DriveCache>();
		auto driveIter = driveCache.find(notification.DriveKey);
		const auto &driveEntry = driveIter.get();
		if (driveEntry.state() != state::DriveState::InProgress)
			return Failure_Service_Drive_Is_Not_In_Progress;

		if (driveEntry.replicators().count(notification.FileRecipient))
			return Failure_Service_Operation_Is_Not_Permitted;

		if (!notification.FileCount)
			return Failure_Service_No_Files_To_Download;

		std::set<Hash256> hashes;
		auto pFile = notification.FilesPtr;
		for (auto i = 0u; i < notification.FileCount; ++i, ++pFile) {
			hashes.insert(pFile->FileHash);
			if (!driveEntry.files().count(pFile->FileHash))
				return Failure_Service_File_Doesnt_Exist;
			if (driveEntry.files().at(pFile->FileHash).Size != pFile->FileSize)
				return Failure_Service_File_Size_Invalid;
		}

		if (hashes.size() != notification.FileCount)
			return Failure_Service_File_Hash_Redundant;

		const auto& downloadCache = context.Cache.sub<cache::DownloadCache>();
		if (downloadCache.contains(notification.DriveKey)) {
			auto downloadCacheIter = downloadCache.find(notification.DriveKey);
			const auto &downloadEntry = downloadCacheIter.get();
			const auto& fileRecipients = downloadEntry.fileRecipients();
			auto fileRecipientIter = fileRecipients.find(notification.FileRecipient);
			if (fileRecipients.end() != fileRecipientIter) {
				const auto& downloads = fileRecipientIter->second;
				auto downloadIter = downloads.find(notification.OperationToken);
				if (downloads.end() != downloadIter) {
					pFile = notification.FilesPtr;
					for (auto i = 0u; i < notification.FileCount; ++i, ++pFile) {
						if (downloadIter->second.count(pFile->FileHash))
							return Failure_Service_File_Download_Already_In_Progress;
					}
				}
			}
		}

		return ValidationResult::Success;
	});
}}
