/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SellOfferBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {

	/// Mixins used by the sell offer cache delta.
	using SellOfferCacheDeltaMixins = PatriciaTreeCacheMixins<SellOfferCacheTypes::PrimaryTypes::BaseSetDeltaType, SellOfferCacheDescriptor>;

	/// Basic delta on top of the sell offer cache.
	class BasicSellOfferCacheDelta
			: public utils::MoveOnly
			, public SellOfferCacheDeltaMixins::Size
			, public SellOfferCacheDeltaMixins::Contains
			, public SellOfferCacheDeltaMixins::ConstAccessor
			, public SellOfferCacheDeltaMixins::MutableAccessor
			, public SellOfferCacheDeltaMixins::PatriciaTreeDelta
			, public SellOfferCacheDeltaMixins::BasicInsertRemove
			, public SellOfferCacheDeltaMixins::DeltaElements
			, public SellOfferCacheDeltaMixins::Enable
			, public SellOfferCacheDeltaMixins::Height {
	public:
		using ReadOnlyView = SellOfferCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a sellOfferSets.
		explicit BasicSellOfferCacheDelta(const SellOfferCacheTypes::BaseSetDeltaPointers& sellOfferSets)
				: SellOfferCacheDeltaMixins::Size(*sellOfferSets.pPrimary)
				, SellOfferCacheDeltaMixins::Contains(*sellOfferSets.pPrimary)
				, SellOfferCacheDeltaMixins::ConstAccessor(*sellOfferSets.pPrimary)
				, SellOfferCacheDeltaMixins::MutableAccessor(*sellOfferSets.pPrimary)
				, SellOfferCacheDeltaMixins::PatriciaTreeDelta(*sellOfferSets.pPrimary, sellOfferSets.pPatriciaTree)
				, SellOfferCacheDeltaMixins::BasicInsertRemove(*sellOfferSets.pPrimary)
				, SellOfferCacheDeltaMixins::DeltaElements(*sellOfferSets.pPrimary)
				, m_pSellOfferEntries(sellOfferSets.pPrimary)
		{}

	public:
		using SellOfferCacheDeltaMixins::ConstAccessor::find;
		using SellOfferCacheDeltaMixins::MutableAccessor::find;

	private:
		SellOfferCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pSellOfferEntries;
	};

	/// Delta on top of the sell offer cache.
	class SellOfferCacheDelta : public ReadOnlyViewSupplier<BasicSellOfferCacheDelta> {
	public:
		/// Creates a delta around \a sellOfferSets.
		explicit SellOfferCacheDelta(const SellOfferCacheTypes::BaseSetDeltaPointers& sellOfferSets)
				: ReadOnlyViewSupplier(sellOfferSets)
		{}
	};
}}
