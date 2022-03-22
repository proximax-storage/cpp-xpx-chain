/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SdaOfferGroupCacheDelta.h"

namespace catapult { namespace cache {

	BasicSdaOfferGroupCacheDelta::BasicSdaOfferGroupCacheDelta(
		const SdaOfferGroupCacheTypes::BaseSetDeltaPointers& sdaOfferGroupSets,
		std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
			: SdaOfferGroupCacheDeltaMixins::Size(*sdaOfferGroupSets.pPrimary)
			, SdaOfferGroupCacheDeltaMixins::Contains(*sdaOfferGroupSets.pPrimary)
			, SdaOfferGroupCacheDeltaMixins::ConstAccessor(*sdaOfferGroupSets.pPrimary)
			, SdaOfferGroupCacheDeltaMixins::MutableAccessor(*sdaOfferGroupSets.pPrimary)
			, SdaOfferGroupCacheDeltaMixins::PatriciaTreeDelta(*sdaOfferGroupSets.pPrimary, sdaOfferGroupSets.pPatriciaTree)
			, SdaOfferGroupCacheDeltaMixins::BasicInsertRemove(*sdaOfferGroupSets.pPrimary)
			, SdaOfferGroupCacheDeltaMixins::Pruning(*sdaOfferGroupSets.pPrimary, *sdaOfferGroupSets.pHeightGrouping)
			, SdaOfferGroupCacheDeltaMixins::DeltaElements(*sdaOfferGroupSets.pPrimary)
			, m_pSdaOfferGroupEntries(sdaOfferGroupSets.pPrimary)
			, m_pHeightGroupingDelta(sdaOfferGroupSets.pHeightGrouping)
	{}

	/// Touches the cache at \a height and returns identifiers of all deactivating elements.
	BasicSdaOfferGroupCacheDelta::SdaOfferGroupHash BasicSdaOfferGroupCacheDelta::getSdaOffersByGroupHash(const Hash256& groupHash) {
		return GetAllIdentifiersWithGroup(*m_pHeightGroupingDelta, groupHash);
	}
}}
