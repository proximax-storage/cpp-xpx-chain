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
#include "MetadataBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/deltaset/BaseSetDelta.h"
#include "plugins/txes/metadata_v2/src/config/MetadataConfiguration.h"

namespace catapult { namespace cache {

	/// Mixins used by the metadata cache delta.
	using MetadataCacheDeltaMixins = PatriciaTreeCacheMixins<MetadataCacheTypes::PrimaryTypes::BaseSetDeltaType, MetadataCacheDescriptor>;

	/// Basic delta on top of the metadata cache.
	class BasicMetadataCacheDelta
			: public utils::MoveOnly
			, public MetadataCacheDeltaMixins::Size
			, public MetadataCacheDeltaMixins::Contains
			, public MetadataCacheDeltaMixins::ConstAccessor
			, public MetadataCacheDeltaMixins::MutableAccessor
			, public MetadataCacheDeltaMixins::PatriciaTreeDelta
			, public MetadataCacheDeltaMixins::BasicInsertRemove
			, public MetadataCacheDeltaMixins::DeltaElements
			, public MetadataCacheDeltaMixins::BroadIteration
			, public MetadataCacheDeltaMixins::ConfigBasedEnable<config::MetadataConfiguration> {
	public:
		using ReadOnlyView = MetadataCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a metadataSets.
		explicit BasicMetadataCacheDelta(
			const MetadataCacheTypes::BaseSetDeltaPointers& metadataSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: MetadataCacheDeltaMixins::Size(*metadataSets.pPrimary)
				, MetadataCacheDeltaMixins::Contains(*metadataSets.pPrimary)
				, MetadataCacheDeltaMixins::ConstAccessor(*metadataSets.pPrimary)
				, MetadataCacheDeltaMixins::MutableAccessor(*metadataSets.pPrimary)
				, MetadataCacheDeltaMixins::PatriciaTreeDelta(*metadataSets.pPrimary, metadataSets.pPatriciaTree)
				, MetadataCacheDeltaMixins::BasicInsertRemove(*metadataSets.pPrimary)
				, MetadataCacheDeltaMixins::DeltaElements(*metadataSets.pPrimary)
				, MetadataCacheDeltaMixins::BroadIteration(*metadataSets.pPrimary)
				, MetadataCacheDeltaMixins::ConfigBasedEnable<config::MetadataConfiguration>(pConfigHolder, [](const auto& config) { return config.Enabled; })
				, m_pMetadataEntries(metadataSets.pPrimary)
		{}

	public:
		using MetadataCacheDeltaMixins::ConstAccessor::find;
		using MetadataCacheDeltaMixins::MutableAccessor::find;

	private:
		MetadataCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pMetadataEntries;
	};

	/// Delta on top of the metadata cache.
	class MetadataCacheDelta : public ReadOnlyViewSupplier<BasicMetadataCacheDelta> {
	public:
		/// Creates a delta around \a metadataSets and \a pConfigHolder.
		explicit MetadataCacheDelta(
			const MetadataCacheTypes::BaseSetDeltaPointers& metadataSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(metadataSets, pConfigHolder)
		{}
	};
}}
