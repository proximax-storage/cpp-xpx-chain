/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "Observers.h"
#include "src/cache/MetadataCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "src/cache/MetadataCacheUtils.h"

namespace catapult { namespace observers {

	namespace {
		void UpdateCache(cache::MetadataCacheDelta& cache, cache::ReadOnlyAccountStateCache accountStateCache, const state::MetadataKey& metadataKey, const RawBuffer& valueBuffer) {
			auto metadataIter = cache::FindEntryKeyIfParticipantsHaveBeenUpgradedByCrawlingHistory(accountStateCache, cache, metadataKey);
			if (!metadataIter.second) {
				auto metadataEntry = state::MetadataEntry(metadataKey);
				metadataEntry.value().update(valueBuffer);
				cache.insert(metadataEntry);
				return;
			}

			if (0 == valueBuffer.Size)
				cache.remove(metadataIter.first.uniqueKey());
			else {
				if(metadataIter.first.uniqueKey() == metadataKey.uniqueKey())
					metadataIter.second->value().update(valueBuffer);
				else {
					cache.remove(metadataIter.first.uniqueKey());
					auto metadataEntry = state::MetadataEntry(metadataIter.first);
					metadataEntry.value().update(valueBuffer);
					cache.insert(metadataEntry);
				}

			}
		}
	}

	DEFINE_OBSERVER(MetadataValue, model::MetadataValueNotification<1>, [](
			const model::MetadataValueNotification<1>& notification,
			const ObserverContext& context) {
		auto& cache = context.Cache.sub<cache::MetadataCache>();
		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>().asReadOnly();

		int32_t valueSize = notification.ValueSize;
		if (NotifyMode::Commit == context.Mode) {
			if (notification.ValueSizeDelta < 0)
				valueSize += notification.ValueSizeDelta;
		} else {
			if (notification.ValueSizeDelta > 0)
				valueSize -= notification.ValueSizeDelta;
		}

		auto metadataKey = state::ResolveMetadataKey(notification.PartialMetadataKey, notification.MetadataTarget, context.Resolvers);
		UpdateCache(cache, accountStateCache, metadataKey, { notification.ValuePtr, static_cast<size_t>(valueSize) });
	})
}}
