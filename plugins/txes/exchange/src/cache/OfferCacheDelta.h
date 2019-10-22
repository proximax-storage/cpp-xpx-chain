/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OfferBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {

	/// Mixins used by the offer cache delta.
	struct OfferCacheDeltaMixins : public PatriciaTreeCacheMixins<OfferCacheTypes::PrimaryTypes::BaseSetDeltaType, OfferCacheDescriptor> {
		using Touch = HeightBasedTouchMixin<
			OfferCacheTypes::PrimaryTypes::BaseSetDeltaType,
			OfferCacheTypes::HeightGroupingTypes::BaseSetDeltaType>;
		using Pruning = HeightBasedPruningMixin<
			OfferCacheTypes::PrimaryTypes::BaseSetDeltaType,
			OfferCacheTypes::HeightGroupingTypes::BaseSetDeltaType>;
	};

	/// Basic delta on top of the offer cache.
	class BasicOfferCacheDelta
			: public utils::MoveOnly
			, public OfferCacheDeltaMixins::Size
			, public OfferCacheDeltaMixins::Contains
			, public OfferCacheDeltaMixins::ConstAccessor
			, public OfferCacheDeltaMixins::MutableAccessor
			, public OfferCacheDeltaMixins::PatriciaTreeDelta
			, public OfferCacheDeltaMixins::BasicInsertRemove
			, public OfferCacheDeltaMixins::Touch
			, public OfferCacheDeltaMixins::Pruning
			, public OfferCacheDeltaMixins::DeltaElements
			, public OfferCacheDeltaMixins::Enable
			, public OfferCacheDeltaMixins::Height {
	public:
		using ReadOnlyView = OfferCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a offerSets.
		explicit BasicOfferCacheDelta(
			const OfferCacheTypes::BaseSetDeltaPointers& offerSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder);

	public:
		using OfferCacheDeltaMixins::ConstAccessor::find;
		using OfferCacheDeltaMixins::MutableAccessor::find;

	public:
		/// Inserts \a value into the cache.
		void insert(const OfferCacheDescriptor::ValueType& value);

		/// Removes the value identified by \a key from the cache.
		void remove(const OfferCacheDescriptor::KeyType& key);

		/// Updates expiry height of offer with \a key from \a currentHeight to \a newHeight.
		void updateExpiryHeight(const OfferCacheDescriptor::KeyType& key, const Height& currentHeight, const Height& newHeight);

		/// Returns \c true if the cache is enabled.
		bool enabled() const;

	private:
		OfferCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pOfferEntries;
		OfferCacheTypes::HeightGroupingTypes::BaseSetDeltaPointerType m_pHeightGroupingDelta;
		std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
	};

	/// Delta on top of the offer cache.
	class OfferCacheDelta : public ReadOnlyViewSupplier<BasicOfferCacheDelta> {
	public:
		/// Creates a delta around \a offerSets.
		explicit OfferCacheDelta(
			const OfferCacheTypes::BaseSetDeltaPointers& offerSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(offerSets, pConfigHolder)
		{}
	};
}}
