/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/MetadataCache.h"

namespace catapult { namespace observers {

	namespace {
		void UpdateCache(cache::MetadataCacheDelta& cache, const state::MetadataKey& metadataKey, bool IsValueImmutable, const RawBuffer& valueBuffer) {
			auto metadataIter = cache.find(metadataKey.uniqueKey());

			if (!metadataIter.tryGet()) {
				auto metadataEntry = state::MetadataEntry(metadataKey);
				metadataEntry.setVersion(2);
				metadataEntry.setImmutable(IsValueImmutable);
				metadataEntry.value().update(valueBuffer);
				cache.insert(metadataEntry);
				return;
			}

			if (0 == valueBuffer.Size)
				cache.remove(metadataKey.uniqueKey());
			else
				metadataIter.get().value().update(valueBuffer);
		}
	}

	DEFINE_OBSERVER(ExtendedMetadataValue, model::MetadataValueNotification<2>, [](
			const model::MetadataValueNotification<2>& notification,
			const ObserverContext& context) {
		auto& cache = context.Cache.sub<cache::MetadataCache>();

		int32_t valueSize = notification.ValueSize;
		if (NotifyMode::Commit == context.Mode) {
			if (notification.ValueSizeDelta < 0)
				valueSize += notification.ValueSizeDelta;
		} else {
			if (notification.ValueSizeDelta > 0)
				valueSize -= notification.ValueSizeDelta;
		}

		auto metadataKey = state::ResolveMetadataKey(notification.PartialMetadataKey, notification.MetadataTarget, context.Resolvers);
		UpdateCache(cache, metadataKey, notification.IsValueImmutable, { notification.ValuePtr, static_cast<size_t>(valueSize) });
	})
}}
