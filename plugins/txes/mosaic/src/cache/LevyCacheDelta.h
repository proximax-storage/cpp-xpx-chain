/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "src/config/MosaicConfiguration.h"
#include "LevyBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {
		
		/// Mixins used by the catapult upgrade cache delta.
		struct LevyCacheDeltaMixins : public PatriciaTreeCacheMixins<LevyCacheTypes::PrimaryTypes::BaseSetDeltaType, LevyCacheDescriptor> {
			using Pruning = HeightBasedPruningMixin<
				LevyCacheTypes::PrimaryTypes::BaseSetDeltaType,
				LevyCacheTypes::HeightGroupingTypes::BaseSetDeltaType>;
		};
		
		/// Basic delta on top of the catapult upgrade cache.
		class BasicLevyCacheDelta
			: public utils::MoveOnly
				, public LevyCacheDeltaMixins::Size
				, public LevyCacheDeltaMixins::Contains
				, public LevyCacheDeltaMixins::ConstAccessor
				, public LevyCacheDeltaMixins::MutableAccessor
				, public LevyCacheDeltaMixins::PatriciaTreeDelta
				, public LevyCacheDeltaMixins::BasicInsertRemove
				, public LevyCacheDeltaMixins::Pruning
				, public LevyCacheDeltaMixins::DeltaElements
				, public LevyCacheDeltaMixins::ConfigBasedEnable<config::MosaicConfiguration> {
		public:
			using ReadOnlyView = LevyCacheTypes::CacheReadOnlyType;
			using CachedMosaicIdByHeight = LevyCacheTypes::HeightGroupingTypes::BaseSetDeltaType::ElementType::Identifiers;
			
		public:
			/// Creates a delta around \a LevySets.
			explicit BasicLevyCacheDelta(const LevyCacheTypes::BaseSetDeltaPointers& LevySets,
				std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: LevyCacheDeltaMixins::Size(*LevySets.pPrimary)
				, LevyCacheDeltaMixins::Contains(*LevySets.pPrimary)
				, LevyCacheDeltaMixins::ConstAccessor(*LevySets.pPrimary)
				, LevyCacheDeltaMixins::MutableAccessor(*LevySets.pPrimary)
				, LevyCacheDeltaMixins::PatriciaTreeDelta(*LevySets.pPrimary, LevySets.pPatriciaTree)
				, LevyCacheDeltaMixins::BasicInsertRemove(*LevySets.pPrimary)
				, LevyCacheDeltaMixins::Pruning(*LevySets.pPrimary, *LevySets.pHistoryAtHeight)
				, LevyCacheDeltaMixins::DeltaElements(*LevySets.pPrimary)
				, LevyCacheDeltaMixins::ConfigBasedEnable<config::MosaicConfiguration>(pConfigHolder, [](const auto& config) { return config.LevyEnabled; })
				, m_pLevyEntries(LevySets.pPrimary)
				, m_pHistoryAtHeight(LevySets.pHistoryAtHeight)
			{}
		
		public:
			using LevyCacheDeltaMixins::ConstAccessor::find;
			using LevyCacheDeltaMixins::MutableAccessor::find;
			
		public:
			
			/// return the cached mosaicIDs by identifier group
			CachedMosaicIdByHeight getCachedMosaicIdsByHeight(const Height& height);
			
			void markHistoryForRemove(const LevyCacheDescriptor::KeyType& key, const Height& height) {
				AddIdentifierWithGroup(*m_pHistoryAtHeight, height, key);
			}
			
			void unmarkHistoryForRemove(const LevyCacheDescriptor::KeyType& key, const Height& height) {
				RemoveIdentifierWithGroup(*m_pHistoryAtHeight, height, key);
			}
			
		private:
			LevyCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pLevyEntries;
			LevyCacheTypes::HeightGroupingTypes::BaseSetDeltaPointerType m_pHistoryAtHeight;
		};
		
		/// Delta on top of the catapult upgrade cache.
		class LevyCacheDelta : public ReadOnlyViewSupplier<BasicLevyCacheDelta> {
		public:
			/// Creates a delta around \a LevySets.
			explicit LevyCacheDelta(const LevyCacheTypes::BaseSetDeltaPointers& LevySets,
				std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(LevySets, pConfigHolder)
			{}
		};
	}}
