/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "LevyBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {
		
		/// Mixins used by the catapult upgrade cache delta.
		using LevyCacheDeltaMixins = PatriciaTreeCacheMixins<LevyCacheTypes::PrimaryTypes::BaseSetDeltaType, LevyCacheDescriptor>;
		
		/// Basic delta on top of the catapult upgrade cache.
		class BasicLevyCacheDelta
			: public utils::MoveOnly
				, public LevyCacheDeltaMixins::Size
				, public LevyCacheDeltaMixins::Contains
				, public LevyCacheDeltaMixins::ConstAccessor
				, public LevyCacheDeltaMixins::MutableAccessor
				, public LevyCacheDeltaMixins::PatriciaTreeDelta
				, public LevyCacheDeltaMixins::BasicInsertRemove
				, public LevyCacheDeltaMixins::DeltaElements
				, public LevyCacheDeltaMixins::Enable
				, public LevyCacheDeltaMixins::Height {
		public:
			using ReadOnlyView = LevyCacheTypes::CacheReadOnlyType;
		
		public:
			/// Creates a delta around \a LevySets.
			explicit BasicLevyCacheDelta(const LevyCacheTypes::BaseSetDeltaPointers& LevySets)
				: LevyCacheDeltaMixins::Size(*LevySets.pPrimary)
				, LevyCacheDeltaMixins::Contains(*LevySets.pPrimary)
				, LevyCacheDeltaMixins::ConstAccessor(*LevySets.pPrimary)
				, LevyCacheDeltaMixins::MutableAccessor(*LevySets.pPrimary)
				, LevyCacheDeltaMixins::PatriciaTreeDelta(*LevySets.pPrimary, LevySets.pPatriciaTree)
				, LevyCacheDeltaMixins::BasicInsertRemove(*LevySets.pPrimary)
				, LevyCacheDeltaMixins::DeltaElements(*LevySets.pPrimary)
				, m_pLevyEntries(LevySets.pPrimary)
			{}
		
		public:
			using LevyCacheDeltaMixins::ConstAccessor::find;
			using LevyCacheDeltaMixins::MutableAccessor::find;
			
		public:
			/// Inserts the mosaic \a entry into the cache.
			void insert(const state::LevyEntry& entry);
			
			/// Removes the value identified by \a mosaicId from the cache.
			void remove(MosaicId mosaicId);
			
		private:
			LevyCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pLevyEntries;
			
		};
		
		/// Delta on top of the catapult upgrade cache.
		class LevyCacheDelta : public ReadOnlyViewSupplier<BasicLevyCacheDelta> {
		public:
			/// Creates a delta around \a LevySets.
			explicit LevyCacheDelta(const LevyCacheTypes::BaseSetDeltaPointers& LevySets)
				: ReadOnlyViewSupplier(LevySets)
			{}
		};
	}}
