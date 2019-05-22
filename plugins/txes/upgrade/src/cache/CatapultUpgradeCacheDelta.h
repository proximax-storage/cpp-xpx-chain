/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CatapultUpgradeBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {

	/// Mixins used by the catapult upgrade cache delta.
	using CatapultUpgradeCacheDeltaMixins = PatriciaTreeCacheMixins<CatapultUpgradeCacheTypes::PrimaryTypes::BaseSetDeltaType, CatapultUpgradeCacheDescriptor>;

	/// Basic delta on top of the catapult upgrade cache.
	class BasicCatapultUpgradeCacheDelta
			: public utils::MoveOnly
			, public CatapultUpgradeCacheDeltaMixins::Size
			, public CatapultUpgradeCacheDeltaMixins::Contains
			, public CatapultUpgradeCacheDeltaMixins::ConstAccessor
			, public CatapultUpgradeCacheDeltaMixins::MutableAccessor
			, public CatapultUpgradeCacheDeltaMixins::PatriciaTreeDelta
			, public CatapultUpgradeCacheDeltaMixins::BasicInsertRemove
			, public CatapultUpgradeCacheDeltaMixins::DeltaElements {
	public:
		using ReadOnlyView = CatapultUpgradeCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a catapultUpgradeSets.
		explicit BasicCatapultUpgradeCacheDelta(const CatapultUpgradeCacheTypes::BaseSetDeltaPointers& catapultUpgradeSets)
				: CatapultUpgradeCacheDeltaMixins::Size(*catapultUpgradeSets.pPrimary)
				, CatapultUpgradeCacheDeltaMixins::Contains(*catapultUpgradeSets.pPrimary)
				, CatapultUpgradeCacheDeltaMixins::ConstAccessor(*catapultUpgradeSets.pPrimary)
				, CatapultUpgradeCacheDeltaMixins::MutableAccessor(*catapultUpgradeSets.pPrimary)
				, CatapultUpgradeCacheDeltaMixins::PatriciaTreeDelta(*catapultUpgradeSets.pPrimary, catapultUpgradeSets.pPatriciaTree)
				, CatapultUpgradeCacheDeltaMixins::BasicInsertRemove(*catapultUpgradeSets.pPrimary)
				, CatapultUpgradeCacheDeltaMixins::DeltaElements(*catapultUpgradeSets.pPrimary)
				, m_pCatapultUpgradeEntries(catapultUpgradeSets.pPrimary)
		{}

	public:
		using CatapultUpgradeCacheDeltaMixins::ConstAccessor::find;
		using CatapultUpgradeCacheDeltaMixins::MutableAccessor::find;

	private:
		CatapultUpgradeCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pCatapultUpgradeEntries;
	};

	/// Delta on top of the catapult upgrade cache.
	class CatapultUpgradeCacheDelta : public ReadOnlyViewSupplier<BasicCatapultUpgradeCacheDelta> {
	public:
		/// Creates a delta around \a catapultUpgradeSets.
		explicit CatapultUpgradeCacheDelta(const CatapultUpgradeCacheTypes::BaseSetDeltaPointers& catapultUpgradeSets)
				: ReadOnlyViewSupplier(catapultUpgradeSets)
		{}
	};
}}
