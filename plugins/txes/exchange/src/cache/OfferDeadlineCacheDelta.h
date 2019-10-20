/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OfferDeadlineBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {

	/// Mixins used by the offer deadline cache delta.
	using OfferDeadlineCacheDeltaMixins = PatriciaTreeCacheMixins<OfferDeadlineCacheTypes::PrimaryTypes::BaseSetDeltaType, OfferDeadlineCacheDescriptor>;

	/// Basic delta on top of the offer deadline cache.
	class BasicOfferDeadlineCacheDelta
			: public utils::MoveOnly
			, public OfferDeadlineCacheDeltaMixins::Size
			, public OfferDeadlineCacheDeltaMixins::Contains
			, public OfferDeadlineCacheDeltaMixins::ConstAccessor
			, public OfferDeadlineCacheDeltaMixins::MutableAccessor
			, public OfferDeadlineCacheDeltaMixins::PatriciaTreeDelta
			, public OfferDeadlineCacheDeltaMixins::BasicInsertRemove
			, public OfferDeadlineCacheDeltaMixins::DeltaElements
			, public OfferDeadlineCacheDeltaMixins::Enable
			, public OfferDeadlineCacheDeltaMixins::Height {
	public:
		using ReadOnlyView = OfferDeadlineCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a offerDeadlineSets.
		explicit BasicOfferDeadlineCacheDelta(const OfferDeadlineCacheTypes::BaseSetDeltaPointers& offerDeadlineSets)
				: OfferDeadlineCacheDeltaMixins::Size(*offerDeadlineSets.pPrimary)
				, OfferDeadlineCacheDeltaMixins::Contains(*offerDeadlineSets.pPrimary)
				, OfferDeadlineCacheDeltaMixins::ConstAccessor(*offerDeadlineSets.pPrimary)
				, OfferDeadlineCacheDeltaMixins::MutableAccessor(*offerDeadlineSets.pPrimary)
				, OfferDeadlineCacheDeltaMixins::PatriciaTreeDelta(*offerDeadlineSets.pPrimary, offerDeadlineSets.pPatriciaTree)
				, OfferDeadlineCacheDeltaMixins::BasicInsertRemove(*offerDeadlineSets.pPrimary)
				, OfferDeadlineCacheDeltaMixins::DeltaElements(*offerDeadlineSets.pPrimary)
				, m_pOfferDeadlineEntries(offerDeadlineSets.pPrimary)
		{}

	public:
		using OfferDeadlineCacheDeltaMixins::ConstAccessor::find;
		using OfferDeadlineCacheDeltaMixins::MutableAccessor::find;

	private:
		OfferDeadlineCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pOfferDeadlineEntries;
	};

	/// Delta on top of the offer deadline cache.
	class OfferDeadlineCacheDelta : public ReadOnlyViewSupplier<BasicOfferDeadlineCacheDelta> {
	public:
		/// Creates a delta around \a offerDeadlineSets.
		explicit OfferDeadlineCacheDelta(const OfferDeadlineCacheTypes::BaseSetDeltaPointers& offerDeadlineSets)
				: ReadOnlyViewSupplier(offerDeadlineSets)
		{}
	};
}}
