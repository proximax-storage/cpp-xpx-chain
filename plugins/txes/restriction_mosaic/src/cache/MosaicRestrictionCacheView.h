/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
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
#include "MosaicRestrictionBaseSets.h"
#include "MosaicRestrictionCacheSerializers.h"
#include "ReadOnlyMosaicRestrictionCache.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"

namespace catapult { namespace cache {

	/// Mixins used by the mosaic restriction cache view.
	struct MosaicRestrictionCacheViewMixins {
	public:
		using PrimaryMixins = PatriciaTreeCacheMixins<MosaicRestrictionCacheTypes::PrimaryTypes::BaseSetType, MosaicRestrictionCacheDescriptor>;
		using ConfigBasedEnable = PrimaryMixins::ConfigBasedEnable<config::MosaicRestrictionConfiguration>;
	};

	/// Basic view on top of the mosaic restriction cache.
	class BasicMosaicRestrictionCacheView
			: public utils::MoveOnly
			, public MosaicRestrictionCacheViewMixins::PrimaryMixins::Size
			, public MosaicRestrictionCacheViewMixins::PrimaryMixins::Contains
			, public MosaicRestrictionCacheViewMixins::PrimaryMixins::Iteration
			, public MosaicRestrictionCacheViewMixins::PrimaryMixins::ConstAccessor
			, public MosaicRestrictionCacheViewMixins::PrimaryMixins::PatriciaTreeView
			, public MosaicRestrictionCacheViewMixins::ConfigBasedEnable {
	public:
		using ReadOnlyView = MosaicRestrictionCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a restrictionSets and \a networkIdentifier.
		BasicMosaicRestrictionCacheView(
				const MosaicRestrictionCacheTypes::BaseSets& restrictionSets,
				std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: MosaicRestrictionCacheViewMixins::PrimaryMixins::Size(restrictionSets.Primary)
				, MosaicRestrictionCacheViewMixins::PrimaryMixins::Contains(restrictionSets.Primary)
				, MosaicRestrictionCacheViewMixins::PrimaryMixins::Iteration(restrictionSets.Primary)
				, MosaicRestrictionCacheViewMixins::PrimaryMixins::ConstAccessor(restrictionSets.Primary)
				, MosaicRestrictionCacheViewMixins::PrimaryMixins::PatriciaTreeView(restrictionSets.PatriciaTree.get())
				, MosaicRestrictionCacheViewMixins::ConfigBasedEnable(pConfigHolder, [](const auto& config) { return config.Enabled; })
				, m_networkIdentifier(pConfigHolder->Config().Immutable.NetworkIdentifier)
		{}

	public:
		/// Gets the network identifier.
		model::NetworkIdentifier networkIdentifier() const {
			return m_networkIdentifier;
		}

	private:
		model::NetworkIdentifier m_networkIdentifier;
	};

	/// View on top of the mosaic restriction cache.
	class MosaicRestrictionCacheView : public ReadOnlyViewSupplier<BasicMosaicRestrictionCacheView> {
	public:
		/// Creates a view around \a restrictionSets and \a networkIdentifier.
		MosaicRestrictionCacheView(
				const MosaicRestrictionCacheTypes::BaseSets& restrictionSets,
				std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(restrictionSets, pConfigHolder)
		{}
	};
}}
