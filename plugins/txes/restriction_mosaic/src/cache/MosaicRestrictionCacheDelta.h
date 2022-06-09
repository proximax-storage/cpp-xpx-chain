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
#include "src/config/MosaicRestrictionConfiguration.h"
#include "MosaicRestrictionBaseSets.h"
#include "ReadOnlyMosaicRestrictionCache.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/deltaset/BaseSetDelta.h"
#include "catapult/model/NetworkInfo.h"

namespace catapult { namespace cache {

	/// Mixins used by the mosaic restriction cache delta.

	struct MosaicRestrictionCacheDeltaMixins {
	public:
		using PrimaryMixins = PatriciaTreeCacheMixins<MosaicRestrictionCacheTypes::PrimaryTypes::BaseSetDeltaType, MosaicRestrictionCacheDescriptor>;
		using ConfigBasedEnable = PrimaryMixins::ConfigBasedEnable<config::MosaicRestrictionConfiguration>;
	};
	/// Basic delta on top of the mosaic restriction cache.
	class BasicMosaicRestrictionCacheDelta
			: public utils::MoveOnly
			, public MosaicRestrictionCacheDeltaMixins::PrimaryMixins::Size
			, public MosaicRestrictionCacheDeltaMixins::PrimaryMixins::Contains
			, public MosaicRestrictionCacheDeltaMixins::PrimaryMixins::ConstAccessor
			, public MosaicRestrictionCacheDeltaMixins::PrimaryMixins::MutableAccessor
			, public MosaicRestrictionCacheDeltaMixins::PrimaryMixins::PatriciaTreeDelta
			, public MosaicRestrictionCacheDeltaMixins::PrimaryMixins::BasicInsertRemove
			, public MosaicRestrictionCacheDeltaMixins::PrimaryMixins::DeltaElements
			, public MosaicRestrictionCacheDeltaMixins::ConfigBasedEnable {
	public:
		using ReadOnlyView = MosaicRestrictionCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a restrictionSets and \a networkIdentifier.
		BasicMosaicRestrictionCacheDelta(
				const MosaicRestrictionCacheTypes::BaseSetDeltaPointers& restrictionSets,
				std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: MosaicRestrictionCacheDeltaMixins::PrimaryMixins::Size(*restrictionSets.pPrimary)
				, MosaicRestrictionCacheDeltaMixins::PrimaryMixins::Contains(*restrictionSets.pPrimary)
				, MosaicRestrictionCacheDeltaMixins::PrimaryMixins::ConstAccessor(*restrictionSets.pPrimary)
				, MosaicRestrictionCacheDeltaMixins::PrimaryMixins::MutableAccessor(*restrictionSets.pPrimary)
				, MosaicRestrictionCacheDeltaMixins::PrimaryMixins::PatriciaTreeDelta(*restrictionSets.pPrimary, restrictionSets.pPatriciaTree)
				, MosaicRestrictionCacheDeltaMixins::PrimaryMixins::BasicInsertRemove(*restrictionSets.pPrimary)
				, MosaicRestrictionCacheDeltaMixins::PrimaryMixins::DeltaElements(*restrictionSets.pPrimary)
				, MosaicRestrictionCacheDeltaMixins::ConfigBasedEnable(pConfigHolder, [](const auto& config) { return config.Enabled; })
				, m_pMosaicRestrictionEntries(restrictionSets.pPrimary)
				, m_networkIdentifier(pConfigHolder->Config().Immutable.NetworkIdentifier)
		{}

	public:
		using MosaicRestrictionCacheDeltaMixins::PrimaryMixins::ConstAccessor::find;
		using MosaicRestrictionCacheDeltaMixins::PrimaryMixins::MutableAccessor::find;

	public:
		/// Gets the network identifier.
		model::NetworkIdentifier networkIdentifier() const {
			return m_networkIdentifier;
		}

	private:
		MosaicRestrictionCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pMosaicRestrictionEntries;
		model::NetworkIdentifier m_networkIdentifier;
	};

	/// Delta on top of the mosaic restriction cache.
	class MosaicRestrictionCacheDelta : public ReadOnlyViewSupplier<BasicMosaicRestrictionCacheDelta> {
	public:
		/// Creates a delta around \a restrictionSets and \a networkIdentifier.
		MosaicRestrictionCacheDelta(
				const MosaicRestrictionCacheTypes::BaseSetDeltaPointers& restrictionSets,
				std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(restrictionSets, pConfigHolder)
		{}
	};
}}
