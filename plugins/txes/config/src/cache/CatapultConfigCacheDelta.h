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
			, public CatapultConfigCacheDeltaMixins::DeltaElements
			, public CatapultConfigCacheDeltaMixins::Enable
			, public CatapultConfigCacheDeltaMixins::Height {
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
				, m_pCatapultConfigHeights(catapultConfigSets.pHeights)
		{}

	public:
		using CatapultConfigCacheDeltaMixins::ConstAccessor::find;
		using CatapultConfigCacheDeltaMixins::MutableAccessor::find;

	public:
		/// Inserts the catapult config \a entry into the cache.
		void insert(const state::CatapultConfigEntry& entry) {
			CatapultConfigCacheDeltaMixins::BasicInsertRemove::insert(entry);
			if (!m_pCatapultConfigHeights->contains(entry.height()))
				m_pCatapultConfigHeights->insert(entry.height());
		}

		/// Removes the catapult config \a entry into the cache.
		void remove(const Height& height) {
			CatapultConfigCacheDeltaMixins::BasicInsertRemove::remove(height);
			if (m_pCatapultConfigHeights->contains(height))
				m_pCatapultConfigHeights->remove(height);
		}

	private:
		CatapultConfigCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pCatapultConfigEntries;
		CatapultConfigCacheTypes::HeightTypes::BaseSetDeltaPointerType m_pCatapultConfigHeights;
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
