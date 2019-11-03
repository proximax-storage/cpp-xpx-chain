/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ExchangeBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {

	/// Mixins used by the exchange cache delta.
	struct ExchangeCacheDeltaMixins : public PatriciaTreeCacheMixins<ExchangeCacheTypes::PrimaryTypes::BaseSetDeltaType, ExchangeCacheDescriptor> {
		using Pruning = HeightBasedPruningMixin<
			ExchangeCacheTypes::PrimaryTypes::BaseSetDeltaType,
			ExchangeCacheTypes::HeightGroupingTypes::BaseSetDeltaType>;
	};

	/// Basic delta on top of the exchange cache.
	class BasicExchangeCacheDelta
			: public utils::MoveOnly
			, public ExchangeCacheDeltaMixins::Size
			, public ExchangeCacheDeltaMixins::Contains
			, public ExchangeCacheDeltaMixins::ConstAccessor
			, public ExchangeCacheDeltaMixins::MutableAccessor
			, public ExchangeCacheDeltaMixins::PatriciaTreeDelta
			, public ExchangeCacheDeltaMixins::BasicInsertRemove
			, public ExchangeCacheDeltaMixins::Pruning
			, public ExchangeCacheDeltaMixins::DeltaElements
			, public ExchangeCacheDeltaMixins::Enable
			, public ExchangeCacheDeltaMixins::Height {
	public:
		using ReadOnlyView = ExchangeCacheTypes::CacheReadOnlyType;
		using OfferOwners = ExchangeCacheTypes::HeightGroupingTypes::BaseSetDeltaType::ElementType::Identifiers;

	public:
		/// Creates a delta around \a exchangeSets.
		explicit BasicExchangeCacheDelta(
			const ExchangeCacheTypes::BaseSetDeltaPointers& exchangeSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder);

	public:
		using ExchangeCacheDeltaMixins::ConstAccessor::find;
		using ExchangeCacheDeltaMixins::MutableAccessor::find;

	public:
		/// Adds offer expiry \a height of \a owner.
		void addExpiryHeight(const ExchangeCacheDescriptor::KeyType& owner, const Height& height);

		/// Removes offer expiry \a height of \a owner.
		void removeExpiryHeight(const ExchangeCacheDescriptor::KeyType& owner, const Height& height);

		/// Returns owners of offers expiring at \a height.
		OfferOwners expiringOfferOwners(Height height);

		/// Returns \c true if the cache is enabled.
		bool enabled() const;

	private:
		ExchangeCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pExchangeEntries;
		ExchangeCacheTypes::HeightGroupingTypes::BaseSetDeltaPointerType m_pHeightGroupingDelta;
		std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
	};

	/// Delta on top of the exchange cache.
	class ExchangeCacheDelta : public ReadOnlyViewSupplier<BasicExchangeCacheDelta> {
	public:
		/// Creates a delta around \a exchangeSets.
		explicit ExchangeCacheDelta(
			const ExchangeCacheTypes::BaseSetDeltaPointers& exchangeSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(exchangeSets, pConfigHolder)
		{}
	};
}}
