/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "LiquidityProviderCacheSerializers.h"
#include "LiquidityProviderCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicLiquidityProviderPatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<LiquidityProviderCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::BaseValueHasher<UnresolvedMosaicId>>;

	class LiquidityProviderPatriciaTree : public BasicLiquidityProviderPatriciaTree {
	public:
		using BasicLiquidityProviderPatriciaTree::BasicLiquidityProviderPatriciaTree;
		using Serializer = LiquidityProviderCacheDescriptor::Serializer;
	};

	struct LiquidityProviderBaseSetDeltaPointers {
		LiquidityProviderCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		LiquidityProviderCacheTypes::KeyTypes::BaseSetDeltaPointerType pDeltaKeys;
		const LiquidityProviderCacheTypes::KeyTypes::BaseSetType& PrimaryKeys;
		std::shared_ptr<LiquidityProviderPatriciaTree::DeltaType> pPatriciaTree;
	};

	struct LiquidityProviderBaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;

	public:
		explicit LiquidityProviderBaseSets(const CacheConfiguration& config)
				: CacheDatabaseMixin(config, { "default" })
				, Primary(GetContainerMode(config), database(), 0)
				, Keys(deltaset::ConditionalContainerMode::Memory, database(), -1)
				, PatriciaTree(hasPatriciaTreeSupport(), database(), 1)
		{}

	public:
		LiquidityProviderCacheTypes::PrimaryTypes::BaseSetType Primary;
		LiquidityProviderCacheTypes::KeyTypes::BaseSetType Keys;
		CachePatriciaTree<LiquidityProviderPatriciaTree> PatriciaTree;

	public:
		LiquidityProviderBaseSetDeltaPointers rebase() {
			return { Primary.rebase(), Keys.rebase(), Keys, PatriciaTree.rebase() };
		}

		LiquidityProviderBaseSetDeltaPointers rebaseDetached() const {
			return {
				Primary.rebaseDetached(),
				Keys.rebaseDetached(),
				Keys,
				PatriciaTree.rebaseDetached()
			};
		}

		void commit() {
			Primary.commit();
			Keys.commit();
			PatriciaTree.commit();
			flush();
		}
	};
}}
