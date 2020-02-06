/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/DownloadCache.h"
#include "src/utils/ServiceUtils.h"
#include "catapult/cache_core/AccountStateCache.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(ExpiredFileDownload, model::BlockNotification<1>, [](const auto&, const ObserverContext& context) {
		auto& downloadCache = context.Cache.sub<cache::DownloadCache>();
		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		auto streamingMosaicId = context.Config.Immutable.StreamingMosaicId;

		downloadCache.processUnusedExpiredLocks(context.Height, [&context, &accountStateCache, streamingMosaicId](const auto& downloadEntry) {
			uint64_t totalSize = 0u;
			for (const auto& pair : downloadEntry.Files) {
				totalSize += pair.second;
			}
			auto amount = utils::CalculateFileDownload(totalSize);
			auto accountStateIter = accountStateCache.find(downloadEntry.FileRecipient);
			auto& accountState = accountStateIter.get();
			if (NotifyMode::Commit == context.Mode)
				accountState.Balances.credit(streamingMosaicId, amount, context.Height);
			else
				accountState.Balances.debit(streamingMosaicId, amount, context.Height);
		});
	});
}}
