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
#include "LockFundCacheSerializers.h"
#include "LockFundCacheTypes.h"
#include "plugins/txes/lock_shared/src/cache/LockInfoBaseSets.h"
#include "LockFundCacheTypes.h"

namespace catapult { namespace cache {

	using BasicLockFundPatriciaTree = tree::BasePatriciaTree<
			SerializerHashedKeyEncoder<LockFundPrimarySerializer>,
			PatriciaTreeRdbDataSource,
			utils::BaseValueHasher<Height>>;

	class LockFundPatriciaTree : public BasicLockFundPatriciaTree {
	public:
		using BasicLockFundPatriciaTree::BasicLockFundPatriciaTree;
		using Serializer = LockFundPrimarySerializer;
	};

	struct LockFundBaseSetDeltaPointers {
		LockFundCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		LockFundCacheTypes::KeyedLockFundTypes::BaseSetDeltaPointerType pKeyedInverseMap;
		std::shared_ptr<LockFundCacheDescriptor::PatriciaTree::DeltaType> pPatriciaTree;
	};

	struct LockFundBaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;

	public:
		explicit LockFundBaseSets(const CacheConfiguration& config)
				: CacheDatabaseMixin(config, { "default", "inverse_map" })
				, Primary(GetContainerMode(config), database(), 0)
				, KeyedInverseMap(GetContainerMode(config), database(), 1)
				, PatriciaTree(hasPatriciaTreeSupport(), database(), 2)
		{}

	public:
		typename LockFundCacheTypes::PrimaryTypes::BaseSetType Primary;
		typename LockFundCacheTypes::KeyedLockFundTypes::BaseSetType KeyedInverseMap;
		CachePatriciaTree<LockFundCacheDescriptor::PatriciaTree> PatriciaTree;

	public:
		LockFundBaseSetDeltaPointers rebase() {
			LockFundBaseSetDeltaPointers deltaPointers;
			deltaPointers.pPrimary = Primary.rebase();
			deltaPointers.pKeyedInverseMap = KeyedInverseMap.rebase();
			deltaPointers.pPatriciaTree = PatriciaTree.rebase();
			return deltaPointers;
		}

		LockFundBaseSetDeltaPointers rebaseDetached() const {
			LockFundBaseSetDeltaPointers deltaPointers;
			deltaPointers.pPrimary = Primary.rebaseDetached();
			deltaPointers.pKeyedInverseMap = KeyedInverseMap.rebaseDetached();
			deltaPointers.pPatriciaTree = PatriciaTree.rebaseDetached();
			return deltaPointers;
		}

		void commit() {
			Primary.commit();
			PatriciaTree.commit();
			KeyedInverseMap.commit();
			flush();
		}
	};
}}
