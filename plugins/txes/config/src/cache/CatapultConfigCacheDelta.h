/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CatapultConfigBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {

	/// Mixins used by the catapult config cache delta.
	using CatapultConfigCacheDeltaMixins = PatriciaTreeCacheMixins<CatapultConfigCacheTypes::PrimaryTypes::BaseSetDeltaType, CatapultConfigCacheDescriptor>;

	/// Basic delta on top of the catapult config cache.
	class BasicCatapultConfigCacheDelta
			: public utils::MoveOnly
			, public CatapultConfigCacheDeltaMixins::Size
			, public CatapultConfigCacheDeltaMixins::Contains
			, public CatapultConfigCacheDeltaMixins::ConstAccessor
			, public CatapultConfigCacheDeltaMixins::MutableAccessor
			, public CatapultConfigCacheDeltaMixins::PatriciaTreeDelta
			, public CatapultConfigCacheDeltaMixins::BasicInsertRemove
			, public CatapultConfigCacheDeltaMixins::DeltaElements {
	public:
		using ReadOnlyView = CatapultConfigCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a catapultConfigSets.
		explicit BasicCatapultConfigCacheDelta(const CatapultConfigCacheTypes::BaseSetDeltaPointers& catapultConfigSets)
				: CatapultConfigCacheDeltaMixins::Size(*catapultConfigSets.pPrimary)
				, CatapultConfigCacheDeltaMixins::Contains(*catapultConfigSets.pPrimary)
				, CatapultConfigCacheDeltaMixins::ConstAccessor(*catapultConfigSets.pPrimary)
				, CatapultConfigCacheDeltaMixins::MutableAccessor(*catapultConfigSets.pPrimary)
				, CatapultConfigCacheDeltaMixins::PatriciaTreeDelta(*catapultConfigSets.pPrimary, catapultConfigSets.pPatriciaTree)
				, CatapultConfigCacheDeltaMixins::BasicInsertRemove(*catapultConfigSets.pPrimary)
				, CatapultConfigCacheDeltaMixins::DeltaElements(*catapultConfigSets.pPrimary)
				, m_pCatapultConfigEntries(catapultConfigSets.pPrimary)
		{}

	public:
		using CatapultConfigCacheDeltaMixins::ConstAccessor::find;
		using CatapultConfigCacheDeltaMixins::MutableAccessor::find;

	private:
		CatapultConfigCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pCatapultConfigEntries;
	};

	/// Delta on top of the catapult config cache.
	class CatapultConfigCacheDelta : public ReadOnlyViewSupplier<BasicCatapultConfigCacheDelta> {
	public:
		/// Creates a delta around \a catapultConfigSets.
		explicit CatapultConfigCacheDelta(const CatapultConfigCacheTypes::BaseSetDeltaPointers& catapultConfigSets)
				: ReadOnlyViewSupplier(catapultConfigSets)
		{}
	};
}}
