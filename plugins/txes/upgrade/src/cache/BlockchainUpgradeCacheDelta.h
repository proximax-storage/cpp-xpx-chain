/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BlockchainUpgradeBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {

	/// Mixins used by the catapult upgrade cache delta.
	using BlockchainUpgradeCacheDeltaMixins = PatriciaTreeCacheMixins<BlockchainUpgradeCacheTypes::PrimaryTypes::BaseSetDeltaType, BlockchainUpgradeCacheDescriptor>;

	/// Basic delta on top of the catapult upgrade cache.
	class BasicBlockchainUpgradeCacheDelta
			: public utils::MoveOnly
			, public BlockchainUpgradeCacheDeltaMixins::Size
			, public BlockchainUpgradeCacheDeltaMixins::Contains
			, public BlockchainUpgradeCacheDeltaMixins::ConstAccessor
			, public BlockchainUpgradeCacheDeltaMixins::MutableAccessor
			, public BlockchainUpgradeCacheDeltaMixins::PatriciaTreeDelta
			, public BlockchainUpgradeCacheDeltaMixins::BasicInsertRemove
			, public BlockchainUpgradeCacheDeltaMixins::DeltaElements
			, public BlockchainUpgradeCacheDeltaMixins::Enable
			, public BlockchainUpgradeCacheDeltaMixins::Height {
	public:
		using ReadOnlyView = BlockchainUpgradeCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a blockchainUpgradeSets.
		explicit BasicBlockchainUpgradeCacheDelta(const BlockchainUpgradeCacheTypes::BaseSetDeltaPointers& blockchainUpgradeSets)
				: BlockchainUpgradeCacheDeltaMixins::Size(*blockchainUpgradeSets.pPrimary)
				, BlockchainUpgradeCacheDeltaMixins::Contains(*blockchainUpgradeSets.pPrimary)
				, BlockchainUpgradeCacheDeltaMixins::ConstAccessor(*blockchainUpgradeSets.pPrimary)
				, BlockchainUpgradeCacheDeltaMixins::MutableAccessor(*blockchainUpgradeSets.pPrimary)
				, BlockchainUpgradeCacheDeltaMixins::PatriciaTreeDelta(*blockchainUpgradeSets.pPrimary, blockchainUpgradeSets.pPatriciaTree)
				, BlockchainUpgradeCacheDeltaMixins::BasicInsertRemove(*blockchainUpgradeSets.pPrimary)
				, BlockchainUpgradeCacheDeltaMixins::DeltaElements(*blockchainUpgradeSets.pPrimary)
				, m_pBlockchainUpgradeEntries(blockchainUpgradeSets.pPrimary)
		{}

	public:
		using BlockchainUpgradeCacheDeltaMixins::ConstAccessor::find;
		using BlockchainUpgradeCacheDeltaMixins::MutableAccessor::find;

	private:
		BlockchainUpgradeCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pBlockchainUpgradeEntries;
	};

	/// Delta on top of the catapult upgrade cache.
	class BlockchainUpgradeCacheDelta : public ReadOnlyViewSupplier<BasicBlockchainUpgradeCacheDelta> {
	public:
		/// Creates a delta around \a blockchainUpgradeSets.
		explicit BlockchainUpgradeCacheDelta(const BlockchainUpgradeCacheTypes::BaseSetDeltaPointers& blockchainUpgradeSets)
				: ReadOnlyViewSupplier(blockchainUpgradeSets)
		{}
	};
}}
