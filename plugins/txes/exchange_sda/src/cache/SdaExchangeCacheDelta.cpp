/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SdaExchangeCacheDelta.h"

namespace catapult { namespace cache {

	BasicSdaExchangeCacheDelta::BasicSdaExchangeCacheDelta(
		const SdaExchangeCacheTypes::BaseSetDeltaPointers& sdaExchangeSets,
		std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
			: SdaExchangeCacheDeltaMixins::Size(*sdaExchangeSets.pPrimary)
			, SdaExchangeCacheDeltaMixins::Contains(*sdaExchangeSets.pPrimary)
			, SdaExchangeCacheDeltaMixins::ConstAccessor(*sdaExchangeSets.pPrimary)
			, SdaExchangeCacheDeltaMixins::MutableAccessor(*sdaExchangeSets.pPrimary)
			, SdaExchangeCacheDeltaMixins::PatriciaTreeDelta(*sdaExchangeSets.pPrimary, sdaExchangeSets.pPatriciaTree)
			, SdaExchangeCacheDeltaMixins::BasicInsertRemove(*sdaExchangeSets.pPrimary)
			, SdaExchangeCacheDeltaMixins::Pruning(*sdaExchangeSets.pPrimary, *sdaExchangeSets.pHeightGrouping)
			, SdaExchangeCacheDeltaMixins::DeltaElements(*sdaExchangeSets.pPrimary)
			, SdaExchangeCacheDeltaMixins::ConfigBasedEnable<config::SdaExchangeConfiguration>(pConfigHolder, [](const auto& config) { return config.Enabled; })
			, m_pSdaExchangeEntries(sdaExchangeSets.pPrimary)
			, m_pHeightGroupingDelta(sdaExchangeSets.pHeightGrouping)
	{}

	void BasicSdaExchangeCacheDelta::addExpiryHeight(const SdaExchangeCacheDescriptor::KeyType& owner, const Height& height) {
		if (state::SdaExchangeEntry::Invalid_Expiry_Height != height)
			AddIdentifierWithGroup(*m_pHeightGroupingDelta, height, owner);
	}

	void BasicSdaExchangeCacheDelta::removeExpiryHeight(const SdaExchangeCacheDescriptor::KeyType& owner, const Height& height) {
		if (state::SdaExchangeEntry::Invalid_Expiry_Height != height)
			RemoveIdentifierWithGroup(*m_pHeightGroupingDelta, height, owner);
	}

	/// Touches the cache at \a height and returns identifiers of all deactivating elements.
	BasicSdaExchangeCacheDelta::SdaOfferOwners BasicSdaExchangeCacheDelta::expiringOfferOwners(Height height) {
		return GetAllIdentifiersWithGroup(*m_pHeightGroupingDelta, height);
	}
}}
