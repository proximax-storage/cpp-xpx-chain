/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "MetadataCacheUtils.h"
#include "MetadataCacheDelta.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "MetadataCacheView.h"

namespace catapult { namespace cache {

	/// Mixins used by the metadata cache view.



	const std::pair<state::MetadataKey, std::optional<state::MetadataEntry>> GetMetadataEntryForStates(const cache::ReadOnlyAccountStateCache& accountStateCache, const cache::ReadOnlyMetadataCache& metadataCache, uint64_t scopedKey, const model::MetadataTarget& target,  const state::AccountState& sourceState, const state::AccountState& targetState) {
		const state::AccountState* targetPtr = &targetState;
		while(targetPtr) {
			auto key = state::CreateMetadataKey(model::PartialMetadataKey{sourceState.Address, targetPtr->PublicKey, scopedKey}, target);
			auto metadataKey = key.uniqueKey();
			if(metadataCache.contains(metadataKey)) {
				return std::make_pair(key, std::make_optional(metadataCache.find(metadataKey).get()));
			}
			if(targetPtr->OldState) {
				targetPtr = &*targetPtr->OldState;
			} else {
				targetPtr = nullptr;
			}
		}
		if(sourceState.OldState) {
			return GetMetadataEntryForStates(accountStateCache, metadataCache, scopedKey, target, *sourceState.OldState, targetState);
		}
		return std::make_pair(state::CreateMetadataKey(model::PartialMetadataKey{sourceState.Address, targetState.PublicKey, scopedKey}, target), std::nullopt);
	}

	const std::pair<state::MetadataKey, std::optional<state::MetadataEntry>> FindEntryKeyIfParticipantsHaveBeenUpgradedByCrawlingHistory(const cache::ReadOnlyAccountStateCache& accountStateCache, const cache::ReadOnlyMetadataCache& metadataCache, const state::MetadataKey& key) {
		auto sourceAcc = accountStateCache.find(key.sourceAddress()).get();
		auto targetAcc = accountStateCache.find(key.targetKey()).get();
		return GetMetadataEntryForStates(accountStateCache, metadataCache, key.scopedMetadataKey(), key.metadataTarget(), sourceAcc, targetAcc);
	}

	const std::pair<state::MetadataKey, state::MetadataEntry*> GetMetadataEntryForStates(const cache::ReadOnlyAccountStateCache& accountStateCache, cache::MetadataCacheDelta& metadataCache, uint64_t scopedKey, const model::MetadataTarget& target,  const state::AccountState& sourceState, const state::AccountState& targetState) {
		const state::AccountState* targetPtr = &targetState;
		while(targetPtr) {
			auto key = state::CreateMetadataKey(model::PartialMetadataKey{sourceState.Address, targetPtr->PublicKey, scopedKey}, target);
			auto metadataKey = key.uniqueKey();
			if(metadataCache.contains(metadataKey)) {
				return std::make_pair(key, &metadataCache.find(metadataKey).get());
			}
			if(targetPtr->OldState) {
				targetPtr = &*targetPtr->OldState;
			} else {
				targetPtr = nullptr;
			}
		}
		if(sourceState.OldState) {
			return GetMetadataEntryForStates(accountStateCache, metadataCache, scopedKey, target, *sourceState.OldState, targetState);
		}
		return std::make_pair(state::CreateMetadataKey(model::PartialMetadataKey{sourceState.Address, targetState.PublicKey, scopedKey}, target), nullptr);
	}
	const std::pair<state::MetadataKey, state::MetadataEntry*> FindEntryKeyIfParticipantsHaveBeenUpgradedByCrawlingHistory(const cache::ReadOnlyAccountStateCache& accountStateCache, cache::MetadataCacheDelta& metadataCache, const state::MetadataKey& key){
		auto sourceAcc = accountStateCache.find(key.sourceAddress()).get();
		auto targetAcc = accountStateCache.find(key.targetKey()).get();
		return GetMetadataEntryForStates(accountStateCache, metadataCache, key.scopedMetadataKey(), key.metadataTarget(), sourceAcc, targetAcc);
	}
}}
