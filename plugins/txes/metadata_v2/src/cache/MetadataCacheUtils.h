/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "MetadataBaseSets.h"
#include "MetadataCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache/CatapultCache.h"

namespace catapult { namespace cache {

	const std::pair<state::MetadataKey, std::optional<state::MetadataEntry>> GetMetadataEntryForStates(const cache::ReadOnlyAccountStateCache& accountStateCache, const cache::ReadOnlyMetadataCache& metadataCache, uint64_t scopedKey, const model::MetadataTarget& target,  const state::AccountState& sourceState, const state::AccountState& targetState);

	const std::pair<state::MetadataKey, std::optional<state::MetadataEntry>> FindEntryKeyIfParticipantsHaveBeenUpgradedByCrawlingHistory(const cache::ReadOnlyAccountStateCache& accountStateCache, const cache::ReadOnlyMetadataCache& metadataCache, const state::MetadataKey& key);

	const std::pair<state::MetadataKey, state::MetadataEntry*> GetMetadataEntryForStates(const cache::ReadOnlyAccountStateCache& accountStateCache, const cache::MetadataCacheDelta& metadataCache, uint64_t scopedKey, const model::MetadataTarget& target,  const state::AccountState& sourceState, const state::AccountState& targetState);

	const std::pair<state::MetadataKey, state::MetadataEntry*> FindEntryKeyIfParticipantsHaveBeenUpgradedByCrawlingHistory(const cache::ReadOnlyAccountStateCache& accountStateCache, cache::MetadataCacheDelta& metadataCache, const state::MetadataKey& key);
}}
