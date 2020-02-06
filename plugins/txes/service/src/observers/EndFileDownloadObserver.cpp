/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommonDrive.h"
#include "src/utils/ServiceUtils.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(EndFileDownload, model::EndFileDownloadNotification<1>, ([](const auto& notification, ObserverContext& context) {
		auto& downloadCache = context.Cache.sub<cache::DownloadCache>();
		auto downloadIter = downloadCache.find(notification.OperationToken);
		auto& downloadEntry = downloadIter.get();

		if (NotifyMode::Commit == context.Mode) {
			auto pFile = notification.FilesPtr;
			for (auto i = 0; i < notification.FileCount; ++i, ++pFile) {
				downloadEntry.Files.erase(pFile->FileHash);
			}
		} else {
			auto pFile = notification.FilesPtr;
			for (auto i = 0; i < notification.FileCount; ++i, ++pFile) {
				downloadEntry.Files.emplace(pFile->FileHash, pFile->FileSize);
			}
		}
	}));
}}
