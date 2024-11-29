/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/MetadataV1Cache.h"

namespace catapult { namespace observers {

	namespace {
		model::MetadataV1ModificationType InvertModificationType(model::MetadataV1ModificationType modificationType) {
			return model::MetadataV1ModificationType::Add == modificationType
					? model::MetadataV1ModificationType::Del
					: model::MetadataV1ModificationType::Add;
		}

		template<typename TNotification>
		void HandleNotifications(const TNotification& notification, const ObserverContext& context) {
			auto& metadataCache = context.Cache.sub<cache::MetadataV1Cache>();
			auto metadataId = state::GetHash(state::ToVector(notification.MetadataId), notification.MetadataType);

			auto metadataIter = metadataCache.find(metadataId);
			if (!metadataIter.tryGet()) {
				metadataCache.insert(state::MetadataV1Entry(state::ToVector(notification.MetadataId), notification.MetadataType));
				metadataIter = metadataCache.find(metadataId);
			}

			auto& metadataEntry = metadataIter.get();

			std::string key(notification.KeyPtr, notification.KeySize);

			auto isCommitMode = NotifyMode::Commit == context.Mode;
			model::MetadataV1ModificationType type = isCommitMode ?
					notification.ModificationType : InvertModificationType(notification.ModificationType);

			for (auto it = metadataEntry.fields().end(); it != metadataEntry.fields().begin();) {
				it--;
				if (it->MetadataKey == key) {
					if (isCommitMode) {
						if (it->RemoveHeight.unwrap() == 0)
							it->RemoveHeight = context.Height;
					} else {
						if (type == model::MetadataV1ModificationType::Del && it->RemoveHeight.unwrap() == 0){
							it = metadataEntry.fields().erase(it);
						} else {
							if (it->RemoveHeight == context.Height) {
								it->RemoveHeight = Height(0);
								break;
							}
						}
					}
				}
			}

			if (isCommitMode && type == model::MetadataV1ModificationType::Add) {
				std::string value(notification.ValuePtr, notification.ValueSize);
				metadataEntry.fields().push_back(state::MetadataV1Field{ key, value, Height(0) });
			}

			if (metadataEntry.fields().empty())
				metadataCache.remove(metadataId);
		}
	}

#define DEFINE_METADATA_MODIFICATION_OBSERVER(OBSERVER_NAME, NOTIFICATION_TYPE) \
	DEFINE_OBSERVER(OBSERVER_NAME, NOTIFICATION_TYPE, [](const auto& notification, const ObserverContext& context) { \
		HandleNotifications<NOTIFICATION_TYPE>(notification, context); \
	});

	DEFINE_METADATA_MODIFICATION_OBSERVER(AddressMetadataV1ValueModification, model::ModifyAddressMetadataValueNotification_v1)
	DEFINE_METADATA_MODIFICATION_OBSERVER(MosaicMetadataV1ValueModification, model::ModifyMosaicMetadataValueNotification_v1)
	DEFINE_METADATA_MODIFICATION_OBSERVER(NamespaceMetadataV1ValueModification, model::ModifyNamespaceMetadataValueNotification_v1)
}}
