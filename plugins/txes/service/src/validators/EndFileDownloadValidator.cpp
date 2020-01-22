/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DownloadCache.h"

namespace catapult { namespace validators {

	using Notification = model::EndFileDownloadNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(EndFileDownload, [](const Notification &notification, const ValidatorContext &context) {
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
					std::set<Hash256> fileHashes;
					auto pFile = notification.FilesPtr;
					for (auto i = 0u; i < notification.FileCount; ++i, ++pFile) {
						fileHashes.insert(pFile->FileHash);
						if (!downloadIter->second.count(pFile->FileHash))
							return Failure_Service_File_Download_Not_In_Progress;
					}

					if (fileHashes.size() != notification.FileCount)
						return Failure_Service_File_Hash_Redundant;

					return ValidationResult::Success;
				}
			}
		}

		return Failure_Service_File_Download_Not_In_Progress;
	});
}}
