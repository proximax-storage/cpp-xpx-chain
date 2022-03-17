/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
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
