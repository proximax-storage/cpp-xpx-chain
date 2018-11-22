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
#include "ReputationCacheTypes.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"

namespace catapult { namespace cache {

	/// Mixins used by the reputation cache view.
	using ReputationCacheViewMixins = BasicCacheMixins<ReputationCacheTypes::PrimaryTypes::BaseSetType, ReputationCacheDescriptor>;

	/// Basic view on top of the reputation cache.
	class BasicReputationCacheView
			: public utils::MoveOnly
			, public ReputationCacheViewMixins::Size
			, public ReputationCacheViewMixins::Contains
			, public ReputationCacheViewMixins::Iteration
			, public ReputationCacheViewMixins::ConstAccessor {
	public:
		using ReadOnlyView = ReputationCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a reputationSets.
		explicit BasicReputationCacheView(const ReputationCacheTypes::BaseSets& reputationSets)
				: ReputationCacheViewMixins::Size(reputationSets.Primary)
				, ReputationCacheViewMixins::Contains(reputationSets.Primary)
				, ReputationCacheViewMixins::Iteration(reputationSets.Primary)
				, ReputationCacheViewMixins::ConstAccessor(reputationSets.Primary)
		{}
	};

	/// View on top of the reputation cache.
	class ReputationCacheView : public ReadOnlyViewSupplier<BasicReputationCacheView> {
	public:
		/// Creates a view around \a reputationSets.
		explicit ReputationCacheView(const ReputationCacheTypes::BaseSets& reputationSets)
				: ReadOnlyViewSupplier(reputationSets)
		{}
	};
}}
