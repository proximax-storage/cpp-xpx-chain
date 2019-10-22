/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "OfferCacheDelta.h"
#include "ExchangeCacheTools.h"
#include "catapult/plugins/PluginUtils.h"

namespace catapult { namespace cache {

	BasicOfferCacheDelta::BasicOfferCacheDelta(
		const OfferCacheTypes::BaseSetDeltaPointers& offerSets,
		std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
			: OfferCacheDeltaMixins::Size(*offerSets.pPrimary)
			, OfferCacheDeltaMixins::Contains(*offerSets.pPrimary)
			, OfferCacheDeltaMixins::ConstAccessor(*offerSets.pPrimary)
			, OfferCacheDeltaMixins::MutableAccessor(*offerSets.pPrimary)
			, OfferCacheDeltaMixins::PatriciaTreeDelta(*offerSets.pPrimary, offerSets.pPatriciaTree)
			, OfferCacheDeltaMixins::BasicInsertRemove(*offerSets.pPrimary)
			, OfferCacheDeltaMixins::Touch(*offerSets.pPrimary, *offerSets.pHeightGrouping)
			, OfferCacheDeltaMixins::Pruning(*offerSets.pPrimary, *offerSets.pHeightGrouping)
			, OfferCacheDeltaMixins::DeltaElements(*offerSets.pPrimary)
			, m_pOfferEntries(offerSets.pPrimary)
			, m_pHeightGroupingDelta(offerSets.pHeightGrouping)
			, m_pConfigHolder(pConfigHolder)
	{}

	void BasicOfferCacheDelta::insert(const OfferCacheDescriptor::ValueType& value) {
		OfferCacheDeltaMixins::BasicInsertRemove::insert(value);
		AddIdentifierWithGroup(*m_pHeightGroupingDelta, value.expiryHeight(), OfferCacheDescriptor::GetKeyFromValue(value));
	}

	void BasicOfferCacheDelta::remove(const OfferCacheDescriptor::KeyType& key) {
		auto iter = m_pOfferEntries->find(key);
		const auto* pOffer = iter.get();
		if (!!pOffer)
			RemoveIdentifierWithGroup(*m_pHeightGroupingDelta, pOffer->expiryHeight(), key);

		OfferCacheDeltaMixins::BasicInsertRemove::remove(key);
	}

	void BasicOfferCacheDelta::updateExpiryHeight(const OfferCacheDescriptor::KeyType& key, const Height& currentHeight, const Height& newHeight) {
		if (currentHeight == newHeight)
			return;

		RemoveIdentifierWithGroup(*m_pHeightGroupingDelta, currentHeight, key);
		AddIdentifierWithGroup(*m_pHeightGroupingDelta, newHeight, key);
	}

	bool BasicOfferCacheDelta::enabled() const {
		return ExchangePluginEnabled(m_pConfigHolder, height());
	}
}}
