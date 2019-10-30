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
		AddIdentifierWithGroup(*m_pHeightGroupingDelta, value.expiryHeight(), ExchangeCacheDescriptor::GetKeyFromValue(value));
	}

	void BasicExchangeCacheDelta::remove(const ExchangeCacheDescriptor::KeyType& key) {
		auto iter = m_pExchangeEntries->find(key);
		const auto* pExchange = iter.get();
		if (!!pExchange)
			RemoveIdentifierWithGroup(*m_pHeightGroupingDelta, pExchange->expiryHeight(), key);

		ExchangeCacheDeltaMixins::BasicInsertRemove::remove(key);
	}

	void BasicExchangeCacheDelta::updateExpiryHeight(const ExchangeCacheDescriptor::KeyType& key, const Height& currentHeight, const Height& newHeight) {
		if (currentHeight == newHeight)
			return;

		RemoveIdentifierWithGroup(*m_pHeightGroupingDelta, currentHeight, key);
		AddIdentifierWithGroup(*m_pHeightGroupingDelta, newHeight, key);
	}

	bool BasicExchangeCacheDelta::enabled() const {
		return ExchangePluginEnabled(m_pConfigHolder, height());
	}
}}
