/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/DownloadCache.h"
#include "src/utils/ServiceUtils.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(StartFileDownload, model::StartFileDownloadNotification<1>, ([](const auto& notification, ObserverContext& context) {
		auto& downloadCache = context.Cache.sub<cache::DownloadCache>();

		if (NotifyMode::Commit == context.Mode) {
			state::DownloadEntry downloadEntry(notification.OperationToken);
			downloadEntry.DriveKey = notification.DriveKey;
			downloadEntry.FileRecipient = notification.FileRecipient;
			const auto& pluginConfig = context.Config.Network.GetPluginConfiguration<config::ServiceConfiguration>();
			downloadEntry.Height = context.Height + Height(pluginConfig.DownloadDuration.unwrap());
			uint64_t totalSize = 0u;
			auto pFile = notification.FilesPtr;
			for (auto i = 0; i < notification.FileCount; ++i, ++pFile) {
				totalSize += pFile->FileSize;
				downloadEntry.Files.emplace(pFile->FileHash, pFile->FileSize);
			}
			downloadCache.insert(downloadEntry);

			model::BalanceChangeReceipt receipt(
				model::Receipt_Type_Drive_Download_Started,
				notification.FileRecipient,
				context.Config.Immutable.StreamingMosaicId,
				utils::CalculateFileDownload(totalSize));
			context.StatementBuilder().addTransactionReceipt(receipt);
		} else {
			downloadCache.remove(notification.OperationToken);
		}
	}));
}}
