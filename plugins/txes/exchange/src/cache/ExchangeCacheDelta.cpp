/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExchangeCacheDelta.h"
#include "ExchangeCacheTools.h"
#include "catapult/plugins/PluginUtils.h"

namespace catapult { namespace cache {

	BasicExchangeCacheDelta::BasicExchangeCacheDelta(
		const ExchangeCacheTypes::BaseSetDeltaPointers& exchangeSets,
		std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
			: ExchangeCacheDeltaMixins::Size(*exchangeSets.pPrimary)
			, ExchangeCacheDeltaMixins::Contains(*exchangeSets.pPrimary)
			, ExchangeCacheDeltaMixins::ConstAccessor(*exchangeSets.pPrimary)
			, ExchangeCacheDeltaMixins::MutableAccessor(*exchangeSets.pPrimary)
			, ExchangeCacheDeltaMixins::PatriciaTreeDelta(*exchangeSets.pPrimary, exchangeSets.pPatriciaTree)
			, ExchangeCacheDeltaMixins::BasicInsertRemove(*exchangeSets.pPrimary)
			, ExchangeCacheDeltaMixins::Pruning(*exchangeSets.pPrimary, *exchangeSets.pHeightGrouping)
			, ExchangeCacheDeltaMixins::DeltaElements(*exchangeSets.pPrimary)
			, m_pExchangeEntries(exchangeSets.pPrimary)
			, m_pHeightGroupingDelta(exchangeSets.pHeightGrouping)
			, m_pConfigHolder(pConfigHolder)
	{}

	void BasicExchangeCacheDelta::addExpiryHeight(const ExchangeCacheDescriptor::KeyType& owner, const Height& height) {
		if (state::ExchangeEntry::Invalid_Expiry_Height != height)
			AddIdentifierWithGroup(*m_pHeightGroupingDelta, height, owner);
	}

	void BasicExchangeCacheDelta::removeExpiryHeight(const ExchangeCacheDescriptor::KeyType& owner, const Height& height) {
		if (state::ExchangeEntry::Invalid_Expiry_Height != height)
			RemoveIdentifierWithGroup(*m_pHeightGroupingDelta, height, owner);
	}

	/// Touches the cache at \a height and returns identifiers of all deactivating elements.
	BasicExchangeCacheDelta::OfferOwners BasicExchangeCacheDelta::expiringOfferOwners(Height height) {
		return GetAllIdentifiersWithGroup(*m_pHeightGroupingDelta, height);
	}

	bool BasicExchangeCacheDelta::enabled() const {
		return ExchangePluginEnabled(m_pConfigHolder, height());
	}
}}
