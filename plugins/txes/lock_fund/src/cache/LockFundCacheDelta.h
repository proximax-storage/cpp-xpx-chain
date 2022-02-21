/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once

#include "catapult/cache/CacheMixinAliases.h"
#include "LockFundBaseSets.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "LockFundCacheTypes.h"
#include "LockFundCacheMixins.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"

namespace catapult { namespace cache {

		struct LockFundCacheDeltaMixins {
		public:
			using PrimaryMixins = PatriciaTreeCacheMixins<LockFundCacheTypes::PrimaryTypes::BaseSetDeltaType, LockFundCacheDescriptor>;
			using KeyedMixins = BasicCacheMixins<LockFundCacheTypes::KeyedLockFundTypes::BaseSetDeltaType, LockFundCacheTypes::KeyedLockFundTypesDescriptor>;
			using LookupMixin = LockFundLookupMixin<LockFundCacheTypes::PrimaryTypes::BaseSetDeltaType, LockFundCacheTypes::KeyedLockFundTypes::BaseSetDeltaType>;
			using Size = LockFundSizeMixin<LockFundCacheTypes::PrimaryTypes::BaseSetDeltaType, LockFundCacheTypes::KeyedLockFundTypes::BaseSetDeltaType>;
		};



		/// Basic delta on top of the namespace cache.
		class BasicLockFundCacheDelta
				: public utils::MoveOnly
						, public LockFundCacheDeltaMixins::Size
						, public LockFundCacheDeltaMixins::PrimaryMixins::Contains
						, public LockFundCacheDeltaMixins::PrimaryMixins::PatriciaTreeDelta
						, public LockFundCacheDeltaMixins::PrimaryMixins::DeltaElements
						, public LockFundCacheDeltaMixins::LookupMixin
						, public LockFundCacheDeltaMixins::KeyedMixins::Contains
						, public LockFundCacheDeltaMixins::PrimaryMixins::Enable
						, public LockFundCacheDeltaMixins::PrimaryMixins::Height {
		public:
			using ReadOnlyView = LockFundCacheTypes::CacheReadOnlyType;
			using CollectedHeights = std::unordered_set<Height, utils::BaseValueHasher<Height>>;
			using LockFundCacheDeltaMixins::KeyedMixins::Contains::contains;
			using LockFundCacheDeltaMixins::PrimaryMixins::Contains::contains;
		public:
			/// Creates a delta around \a lockFundSets.
			BasicLockFundCacheDelta(
					const LockFundCacheTypes::BaseSetDeltaPointers& lockFundSets);

		public:
			/// Inserts the lockfund record \a ns into the cache.
			void insert(const Key& publicKey, Height unlockHeight, const std::map<MosaicId, Amount>& mosaics);

			/// Removes the lock fund record specified by its \a publicKey and \a height from the cache.
			void remove(const Key& publicKey, Height height);

			/// Tags as inactive the lock fund record specified by its \a publicKey and \a height from the cache.
			void disable(const Key& publicKey, Height height);

			/// Inserts the lockfund record \a ns into the cache by copying an existing record. Will not validate!
			void insert(const state::LockFundRecordGroup<LockFundHeightIndexDescriptor>& record);

			/// Reactivates the lock fund record specified by its \a publicKey and \a height from the cache.
			void enable(const Key& publicKey, Height height);

			/// Tags as inactive the records in the lock fund record group specified by its \a height.
			void remove(Height height);

			/// Reactivates the records in the lock fund record group specified by its \a height.
			void recover(Height height);

			/// Removes the lock fund record group specified by its \a height.
			void prune(Height height);

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
					const LockFundCacheTypes::BaseSetDeltaPointers& namespaceSets)
					: ReadOnlyViewSupplier(namespaceSets)
			{}
		};
}}
