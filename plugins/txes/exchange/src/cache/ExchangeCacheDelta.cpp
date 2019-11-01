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
			, ExchangeCacheDeltaMixins::Touch(*exchangeSets.pPrimary, *exchangeSets.pHeightGrouping)
			, ExchangeCacheDeltaMixins::Pruning(*exchangeSets.pPrimary, *exchangeSets.pHeightGrouping)
			, ExchangeCacheDeltaMixins::DeltaElements(*exchangeSets.pPrimary)
			, m_pExchangeEntries(exchangeSets.pPrimary)
			, m_pHeightGroupingDelta(exchangeSets.pHeightGrouping)
			, m_pConfigHolder(pConfigHolder)
	{}

	void BasicExchangeCacheDelta::insert(const ExchangeCacheDescriptor::ValueType& value) {
		ExchangeCacheDeltaMixins::BasicInsertRemove::insert(value);
		auto expiryHeight = value.minExpiryHeight();
		if (state::ExchangeEntry::Invalid_Expiry_Height != expiryHeight)
			AddIdentifierWithGroup(*m_pHeightGroupingDelta, expiryHeight, ExchangeCacheDescriptor::GetKeyFromValue(value));
	}

	void BasicExchangeCacheDelta::updateExpiryHeight(const ExchangeCacheDescriptor::KeyType& key, const Height& currentHeight, const Height& newHeight) {
		if (currentHeight == newHeight)
			return;

		if (state::ExchangeEntry::Invalid_Expiry_Height != currentHeight)
			RemoveIdentifierWithGroup(*m_pHeightGroupingDelta, currentHeight, key);
		if (state::ExchangeEntry::Invalid_Expiry_Height != newHeight)
			AddIdentifierWithGroup(*m_pHeightGroupingDelta, newHeight, key);
	}

	bool BasicExchangeCacheDelta::enabled() const {
		return ExchangePluginEnabled(m_pConfigHolder, height());
	}
}}
