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
				, m_pDeltaHeights(catapultConfigSets.pDeltaHeights)
				, m_PrimaryHeights(catapultConfigSets.PrimaryHeights)
		{}

	public:
		using CatapultConfigCacheDeltaMixins::ConstAccessor::find;
		using CatapultConfigCacheDeltaMixins::MutableAccessor::find;

	public:
		/// Inserts the catapult config \a entry into the cache.
		void insert(const state::CatapultConfigEntry& entry) {
			CatapultConfigCacheDeltaMixins::BasicInsertRemove::insert(entry);
			insertHeight(entry.height());
		}

		/// Inserts the \a height of catapult config into the cache.
		void insertHeight(const Height& height) {
			if (!m_pDeltaHeights->contains(height))
				m_pDeltaHeights->insert(height);
		}

		/// Removes the catapult config \a entry into the cache.
		void remove(const Height& height) {
			CatapultConfigCacheDeltaMixins::BasicInsertRemove::remove(height);
			if (m_pDeltaHeights->contains(height))
				m_pDeltaHeights->remove(height);
		}

		/// Returns heights available after commit
		std::set<Height> heights() const {
			std::set<Height> result;
			for (const auto& height : deltaset::MakeIterableView(m_PrimaryHeights)) {
				result.insert(height);
			}

			auto deltas = m_pDeltaHeights->deltas();

			for (const auto& height : deltas.Added) {
				result.insert(height);
			}

			for (const auto& height : deltas.Removed) {
				result.erase(height);
			}

			return result;
		}

	private:
		CatapultConfigCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pCatapultConfigEntries;
		CatapultConfigCacheTypes::HeightTypes::BaseSetDeltaPointerType m_pDeltaHeights;
		const CatapultConfigCacheTypes::HeightTypes::BaseSetType& m_PrimaryHeights;
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
