/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <src/catapult/validators/ValidatorContext.h>
#include "Validators.h"
#include "plugins/txes/metadata/src/model/MetadataTypes.h"
#include "plugins/txes/metadata/src/cache/MetadataCache.h"

namespace catapult { namespace validators {

	using Notification = model::MetadataModificationsNotification;


	namespace {
		ValidationResult validate(const Notification& notification, const ValidatorContext& context, const uint8_t& maxFields) {
			const auto& metadataCache = context.Cache.sub<cache::MetadataCache>();
			auto it = metadataCache.find(notification.MetadataId);

			state::MetadataEntry metadataEntry;

			if (it.tryGet())
				metadataEntry = it.get();

			std::unordered_map<size_t, size_t> added;
			std::unordered_set<size_t> removed;

			std::hash<std::string> hasher;
			for (const auto& modification : notification.Modifications) {
				auto key = hasher(std::string(modification->KeyPtr(), modification->KeySize));

				if (modification->ModificationType == model::MetadataModificationType::Add && removed.count(key) == 0) {
					auto value = hasher(std::string(modification->ValuePtr(), modification->ValueSize));
					added.emplace(key, value);
				} else if (modification->ModificationType == model::MetadataModificationType::Del && added.count(key) == 0) {
					removed.emplace(key);
				}
			}

			if (added.size() + removed.size() != notification.Modifications.size())
				return Failure_Metadata_Modification_Key_Redundant;

			for (const auto& field : metadataEntry.fields()) {
				if (field.RemoveHeight.unwrap() != 0)
					continue;

				auto key = hasher(field.MetadataKey);

				// we want to check, that we have all keys which we want to remove
				if (removed.count(key)) {
					removed.erase(key);
					continue;
				}

				// if we already have the same key with the same value, we failure
				if (added.count(key) && added[key] == hasher(field.MetadataValue))
					return Failure_Metadata_Modification_Value_Redundant;

				// we add key from field to the map because we want to calculate the count of unique fields after modifications
				added[key] = 0;
			}

			if (added.size() > maxFields)
				return Failure_Metadata_Too_Much_Keys;

			if (!removed.empty())
				return Failure_Metadata_Remove_Not_Existing_Key;

			return ValidationResult::Success;
		}
	}

	DECLARE_STATEFUL_VALIDATOR(MetadataModifications, Notification)(const uint8_t& maxFields) {
		return MAKE_STATEFUL_VALIDATOR(MetadataModifications, [maxFields](const Notification& notification, const ValidatorContext& context){
			return validate(notification, context, maxFields);
		});
	}
}}
