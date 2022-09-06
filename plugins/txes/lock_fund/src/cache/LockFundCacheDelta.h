/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "src/config/LockFundConfiguration.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "LockFundBaseSets.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "LockFundCacheTypes.h"
#include "LockFundCacheMixins.h"
#include "catapult/deltaset/DeltaElementsMixin.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"

namespace catapult { namespace cache {

		struct LockFundCacheDeltaMixins {
		public:
			using PrimaryMixins = PatriciaTreeCacheMixins<LockFundCacheTypes::PrimaryTypes::BaseSetDeltaType, LockFundCacheDescriptor>;
			using KeyedMixins = BasicCacheMixins<LockFundCacheTypes::KeyedLockFundTypes::BaseSetDeltaType, LockFundCacheTypes::KeyedLockFundTypesDescriptor>;
			//Delta helper functions must be used in order to keep multi-set stability.
			//using LookupMixin = LockFundMutableLookupMixin<LockFundCacheTypes::PrimaryTypes::BaseSetDeltaType, LockFundCacheTypes::KeyedLockFundTypes::BaseSetDeltaType>;
			using ReadOnlyLookupMixin = LockFundConstLookupMixin<LockFundCacheTypes::PrimaryTypes::BaseSetDeltaType, LockFundCacheTypes::KeyedLockFundTypes::BaseSetDeltaType>;
			using Size = LockFundSizeMixin<LockFundCacheTypes::PrimaryTypes::BaseSetDeltaType, LockFundCacheTypes::KeyedLockFundTypes::BaseSetDeltaType>;
			using DeltaElements = deltaset::DeltaElementsMultiSetMixin<LockFundCacheTypes::PrimaryTypes::BaseSetDeltaType, LockFundCacheTypes::KeyedLockFundTypes::BaseSetDeltaType>;
			using ConfigBasedEnable = PrimaryMixins::ConfigBasedEnable<config::LockFundConfiguration>;
		};



		/// Basic delta on top of the namespace cache.
		class BasicLockFundCacheDelta
				: public utils::MoveOnly
						, public LockFundCacheDeltaMixins::Size
						, public LockFundCacheDeltaMixins::PrimaryMixins::Contains
						, public LockFundCacheDeltaMixins::PrimaryMixins::PatriciaTreeDelta
						, public LockFundCacheDeltaMixins::DeltaElements
						, public LockFundCacheDeltaMixins::ReadOnlyLookupMixin
						, public LockFundCacheDeltaMixins::KeyedMixins::Contains
						, public LockFundCacheDeltaMixins::ConfigBasedEnable{
		public:
			using ReadOnlyView = LockFundCacheTypes::CacheReadOnlyType;
			using LockFundCacheDeltaMixins::KeyedMixins::Contains::contains;
			using LockFundCacheDeltaMixins::PrimaryMixins::Contains::contains;
			using LockFundCacheDeltaMixins::ReadOnlyLookupMixin::find;
		public:
			/// Creates a delta around \a lockFundSets.
			BasicLockFundCacheDelta(
					const LockFundCacheTypes::BaseSetDeltaPointers& lockFundSets,
					std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder);

		public:
			/// Inserts the lockfund record \a ns into the cache.
			void insert(const Key& publicKey, Height unlockHeight, const std::map<MosaicId, Amount>& mosaics);

			/// Removes the lock fund record specified by its \a publicKey and \a height from the cache.
			void remove(const Key& publicKey, Height height);

			/// Tags as inactive the lock fund record specified by its \a publicKey and \a height from the cache.
			void disable(const Key& publicKey, Height height);

			/// Inserts the lockfund record \a ns into the cache by copying an existing record. Will not validate!
			void insert(const state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>& record);

			/// Reactivates the lock fund record specified by its \a publicKey and \a height from the cache.
			void recover(const Key& publicKey, Height height);

			/// Tags as inactive the records in the lock fund record group specified by its \a height.
			void disable(Height height);

			/// Reactivates the records in the lock fund record group specified by its \a height.
			void recover(Height height);

			/// Removes the lock fund record group specified by its \a height.
			void remove(Height height);

			/// Acts on the lock fund group and then prunes the height
			template<typename TAction>
			void actAndToggle(Height height, bool isActive, TAction action)
			{
				auto heightGroupIterator = m_pLockFundGroupsByHeight->find(height);
				auto* heightGroup = heightGroupIterator.get();
				if(heightGroup)
				{
					for(auto& tiedRecord : heightGroup->LockFundRecords)
					{
						auto keyGroupIterator = m_pLockFundGroupsByKey->find(tiedRecord.first);
						auto* keyGroup = keyGroupIterator.get();
						if(isActive)
						{
							keyGroup->LockFundRecords[height].Reactivate();
							tiedRecord.second.Reactivate();
						}

						if(tiedRecord.second.Active())
							action(tiedRecord.first, tiedRecord.second.Get());

						if(!isActive)
						{
							keyGroup->LockFundRecords[height].Inactivate();
							tiedRecord.second.Inactivate();
						}
					}
				}
			}

		private:
			LockFundCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pLockFundGroupsByHeight;
			LockFundCacheTypes::KeyedLockFundTypes::BaseSetDeltaPointerType m_pLockFundGroupsByKey;
		};

		/// Delta on top of the namespace cache.
		class LockFundCacheDelta : public ReadOnlyViewSupplier<BasicLockFundCacheDelta> {

		public:
			/// Creates a delta around \a namespaceSets, \a options and \a namespaceSizes.
			LockFundCacheDelta(
					const LockFundCacheTypes::BaseSetDeltaPointers& namespaceSets,
					std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
					: ReadOnlyViewSupplier(namespaceSets, pConfigHolder)
			{}
		};
}}
