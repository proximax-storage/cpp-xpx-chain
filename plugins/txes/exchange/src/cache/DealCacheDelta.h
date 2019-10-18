/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DealBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {

	/// Mixins used by the deal cache delta.
	using DealCacheDeltaMixins = PatriciaTreeCacheMixins<DealCacheTypes::PrimaryTypes::BaseSetDeltaType, DealCacheDescriptor>;

	/// Basic delta on top of the deal cache.
	class BasicDealCacheDelta
			: public utils::MoveOnly
			, public DealCacheDeltaMixins::Size
			, public DealCacheDeltaMixins::Contains
			, public DealCacheDeltaMixins::ConstAccessor
			, public DealCacheDeltaMixins::MutableAccessor
			, public DealCacheDeltaMixins::PatriciaTreeDelta
			, public DealCacheDeltaMixins::BasicInsertRemove
			, public DealCacheDeltaMixins::DeltaElements
			, public DealCacheDeltaMixins::Enable
			, public DealCacheDeltaMixins::Height {
	public:
		using ReadOnlyView = DealCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a dealSets.
		explicit BasicDealCacheDelta(const DealCacheTypes::BaseSetDeltaPointers& dealSets)
				: DealCacheDeltaMixins::Size(*dealSets.pPrimary)
				, DealCacheDeltaMixins::Contains(*dealSets.pPrimary)
				, DealCacheDeltaMixins::ConstAccessor(*dealSets.pPrimary)
				, DealCacheDeltaMixins::MutableAccessor(*dealSets.pPrimary)
				, DealCacheDeltaMixins::PatriciaTreeDelta(*dealSets.pPrimary, dealSets.pPatriciaTree)
				, DealCacheDeltaMixins::BasicInsertRemove(*dealSets.pPrimary)
				, DealCacheDeltaMixins::DeltaElements(*dealSets.pPrimary)
				, m_pDealEntries(dealSets.pPrimary)
		{}

	public:
		using DealCacheDeltaMixins::ConstAccessor::find;
		using DealCacheDeltaMixins::MutableAccessor::find;

	private:
		DealCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pDealEntries;
	};

	/// Delta on top of the deal cache.
	class DealCacheDelta : public ReadOnlyViewSupplier<BasicDealCacheDelta> {
	public:
		/// Creates a delta around \a dealSets.
		explicit DealCacheDelta(const DealCacheTypes::BaseSetDeltaPointers& dealSets)
				: ReadOnlyViewSupplier(dealSets)
		{}
	};
}}
