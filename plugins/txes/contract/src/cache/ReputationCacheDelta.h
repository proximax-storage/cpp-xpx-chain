/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ReputationBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {

	/// Mixins used by the reputation cache delta.
	using ReputationCacheDeltaMixins = PatriciaTreeCacheMixins<ReputationCacheTypes::PrimaryTypes::BaseSetDeltaType, ReputationCacheDescriptor>;

	/// Basic delta on top of the Reputation cache.
	class BasicReputationCacheDelta
			: public utils::MoveOnly
			, public ReputationCacheDeltaMixins::Size
			, public ReputationCacheDeltaMixins::Contains
			, public ReputationCacheDeltaMixins::ConstAccessor
			, public ReputationCacheDeltaMixins::MutableAccessor
			, public ReputationCacheDeltaMixins::PatriciaTreeDelta
			, public ReputationCacheDeltaMixins::BasicInsertRemove
			, public ReputationCacheDeltaMixins::DeltaElements
			, public ReputationCacheDeltaMixins::Enable
			, public ReputationCacheDeltaMixins::Height {
	public:
		using ReadOnlyView = ReputationCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a reputationSets.
		explicit BasicReputationCacheDelta(const ReputationCacheTypes::BaseSetDeltaPointers& reputationSets)
				: ReputationCacheDeltaMixins::Size(*reputationSets.pPrimary)
				, ReputationCacheDeltaMixins::Contains(*reputationSets.pPrimary)
				, ReputationCacheDeltaMixins::ConstAccessor(*reputationSets.pPrimary)
				, ReputationCacheDeltaMixins::MutableAccessor(*reputationSets.pPrimary)
				, ReputationCacheDeltaMixins::PatriciaTreeDelta(*reputationSets.pPrimary, reputationSets.pPatriciaTree)
				, ReputationCacheDeltaMixins::BasicInsertRemove(*reputationSets.pPrimary)
				, ReputationCacheDeltaMixins::DeltaElements(*reputationSets.pPrimary)
				, m_pReputationEntries(reputationSets.pPrimary)
		{}

	public:
		using ReputationCacheDeltaMixins::ConstAccessor::find;
		using ReputationCacheDeltaMixins::MutableAccessor::find;

		void setEnabled(bool) {
		}

		/// Disable the cache.
		/// TODO: remove the contract plugin.
		bool enabled() const {
			return false;
		}

	private:
		ReputationCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pReputationEntries;
	};

	/// Delta on top of the reputation cache.
	class ReputationCacheDelta : public ReadOnlyViewSupplier<BasicReputationCacheDelta> {
	public:
		/// Creates a delta around \a reputationSets.
		explicit ReputationCacheDelta(const ReputationCacheTypes::BaseSetDeltaPointers& reputationSets)
				: ReadOnlyViewSupplier(reputationSets)
		{}
	};
}}
