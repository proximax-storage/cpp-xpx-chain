/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BlockchainUpgradeBaseSets.h"
#include "BlockchainUpgradeCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"

namespace catapult { namespace cache {

	/// Mixins used by the catapult upgrade cache view.
	using BlockchainUpgradeCacheViewMixins = PatriciaTreeCacheMixins<BlockchainUpgradeCacheTypes::PrimaryTypes::BaseSetType, BlockchainUpgradeCacheDescriptor>;

	/// Basic view on top of the catapult upgrade cache.
	class BasicBlockchainUpgradeCacheView
			: public utils::MoveOnly
			, public BlockchainUpgradeCacheViewMixins::Size
			, public BlockchainUpgradeCacheViewMixins::Contains
			, public BlockchainUpgradeCacheViewMixins::Iteration
			, public BlockchainUpgradeCacheViewMixins::ConstAccessor
			, public BlockchainUpgradeCacheViewMixins::PatriciaTreeView
			, public BlockchainUpgradeCacheViewMixins::Enable
			, public BlockchainUpgradeCacheViewMixins::Height {
	public:
		using ReadOnlyView = BlockchainUpgradeCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a blockchainUpgradeSets.
		explicit BasicBlockchainUpgradeCacheView(const BlockchainUpgradeCacheTypes::BaseSets& blockchainUpgradeSets)
				: BlockchainUpgradeCacheViewMixins::Size(blockchainUpgradeSets.Primary)
				, BlockchainUpgradeCacheViewMixins::Contains(blockchainUpgradeSets.Primary)
				, BlockchainUpgradeCacheViewMixins::Iteration(blockchainUpgradeSets.Primary)
				, BlockchainUpgradeCacheViewMixins::ConstAccessor(blockchainUpgradeSets.Primary)
				, BlockchainUpgradeCacheViewMixins::PatriciaTreeView(blockchainUpgradeSets.PatriciaTree.get())
		{}
	};

	/// View on top of the catapult upgrade cache.
	class BlockchainUpgradeCacheView : public ReadOnlyViewSupplier<BasicBlockchainUpgradeCacheView> {
	public:
		/// Creates a view around \a blockchainUpgradeSets.
		explicit BlockchainUpgradeCacheView(const BlockchainUpgradeCacheTypes::BaseSets& blockchainUpgradeSets)
				: ReadOnlyViewSupplier(blockchainUpgradeSets)
		{}
	};
}}
