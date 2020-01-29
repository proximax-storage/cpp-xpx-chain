/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DownloadCache.h"

namespace catapult { namespace validators {

	using Notification = model::EndFileDownloadNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(EndFileDownload, [](const Notification& notification, const ValidatorContext& context) {
		if (!notification.FileCount)
			return Failure_Service_No_Files_To_Download;

		const auto& downloadCache = context.Cache.sub<cache::DownloadCache>();
		if (downloadCache.contains(notification.OperationToken)) {
			auto downloadCacheIter = downloadCache.find(notification.OperationToken);
			const auto& downloadEntry = downloadCacheIter.get();

			if (downloadEntry.FileRecipient != notification.FileRecipient)
				return Failure_Service_Invalid_File_Recipient;

			if (downloadEntry.Height >= context.Height) {
				std::set<Hash256> fileHashes;
				auto pFile = notification.FilesPtr;
				for (auto i = 0u; i < notification.FileCount; ++i, ++pFile) {
					fileHashes.insert(pFile->FileHash);
					if (!downloadEntry.Files.count(pFile->FileHash))
						return Failure_Service_File_Download_Not_In_Progress;
				}

				if (fileHashes.size() != notification.FileCount)
					return Failure_Service_File_Hash_Redundant;

				return ValidationResult::Success;
			}
		}

		return Failure_Service_File_Download_Not_In_Progress;
	});
}}
