/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BuyOfferBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {

	/// Mixins used by the buy offer cache delta.
	using BuyOfferCacheDeltaMixins = PatriciaTreeCacheMixins<BuyOfferCacheTypes::PrimaryTypes::BaseSetDeltaType, BuyOfferCacheDescriptor>;

	/// Basic delta on top of the buy offer cache.
	class BasicBuyOfferCacheDelta
			: public utils::MoveOnly
			, public BuyOfferCacheDeltaMixins::Size
			, public BuyOfferCacheDeltaMixins::Contains
			, public BuyOfferCacheDeltaMixins::ConstAccessor
			, public BuyOfferCacheDeltaMixins::MutableAccessor
			, public BuyOfferCacheDeltaMixins::PatriciaTreeDelta
			, public BuyOfferCacheDeltaMixins::BasicInsertRemove
			, public BuyOfferCacheDeltaMixins::DeltaElements
			, public BuyOfferCacheDeltaMixins::Enable
			, public BuyOfferCacheDeltaMixins::Height {
	public:
		using ReadOnlyView = BuyOfferCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a buyOfferSets.
		explicit BasicBuyOfferCacheDelta(const BuyOfferCacheTypes::BaseSetDeltaPointers& buyOfferSets)
				: BuyOfferCacheDeltaMixins::Size(*buyOfferSets.pPrimary)
				, BuyOfferCacheDeltaMixins::Contains(*buyOfferSets.pPrimary)
				, BuyOfferCacheDeltaMixins::ConstAccessor(*buyOfferSets.pPrimary)
				, BuyOfferCacheDeltaMixins::MutableAccessor(*buyOfferSets.pPrimary)
				, BuyOfferCacheDeltaMixins::PatriciaTreeDelta(*buyOfferSets.pPrimary, buyOfferSets.pPatriciaTree)
				, BuyOfferCacheDeltaMixins::BasicInsertRemove(*buyOfferSets.pPrimary)
				, BuyOfferCacheDeltaMixins::DeltaElements(*buyOfferSets.pPrimary)
				, m_pBuyOfferEntries(buyOfferSets.pPrimary)
		{}

	public:
		using BuyOfferCacheDeltaMixins::ConstAccessor::find;
		using BuyOfferCacheDeltaMixins::MutableAccessor::find;

	private:
		BuyOfferCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pBuyOfferEntries;
	};

	/// Delta on top of the buy offer cache.
	class BuyOfferCacheDelta : public ReadOnlyViewSupplier<BasicBuyOfferCacheDelta> {
	public:
		/// Creates a delta around \a buyOfferSets.
		explicit BuyOfferCacheDelta(const BuyOfferCacheTypes::BaseSetDeltaPointers& buyOfferSets)
				: ReadOnlyViewSupplier(buyOfferSets)
		{}
	};
}}
