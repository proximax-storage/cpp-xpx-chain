/**
*** Copyright (c) 2018-present,
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
			, public ReputationCacheDeltaMixins::DeltaElements {
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
