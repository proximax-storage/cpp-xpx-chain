/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DriveBaseSets.h"
#include "ServiceCacheTools.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {

	/// Mixins used by the drive delta view.
	struct DriveCacheDeltaMixins {
	private:
		using PrimaryMixins = PatriciaTreeCacheMixins<DriveCacheTypes::PrimaryTypes::BaseSetDeltaType, DriveCacheDescriptor>;

	public:
		using Size = PrimaryMixins::Size;
		using Contains = PrimaryMixins::Contains;
		using PatriciaTreeDelta = PrimaryMixins::PatriciaTreeDelta;
		using MutableAccessor = PrimaryMixins::ConstAccessor;
		using ConstAccessor = PrimaryMixins::MutableAccessor;
		using DeltaElements = PrimaryMixins::DeltaElements;
		using BasicInsertRemove = PrimaryMixins::BasicInsertRemove;
		using Enable = PrimaryMixins::Enable;
		using Height = PrimaryMixins::Height;
	};

	/// Basic delta on top of the drive cache.
	class BasicDriveCacheDelta
			: public utils::MoveOnly
			, public DriveCacheDeltaMixins::Size
			, public DriveCacheDeltaMixins::Contains
			, public DriveCacheDeltaMixins::ConstAccessor
			, public DriveCacheDeltaMixins::MutableAccessor
			, public DriveCacheDeltaMixins::PatriciaTreeDelta
			, public DriveCacheDeltaMixins::BasicInsertRemove
			, public DriveCacheDeltaMixins::DeltaElements
			, public DriveCacheDeltaMixins::Enable
			, public DriveCacheDeltaMixins::Height {
	public:
		using ReadOnlyView = DriveCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a driveSets and \a pConfigHolder.
		explicit BasicDriveCacheDelta(
			const DriveCacheTypes::BaseSetDeltaPointers& driveSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: DriveCacheDeltaMixins::Size(*driveSets.pPrimary)
				, DriveCacheDeltaMixins::Contains(*driveSets.pPrimary)
				, DriveCacheDeltaMixins::ConstAccessor(*driveSets.pPrimary)
				, DriveCacheDeltaMixins::MutableAccessor(*driveSets.pPrimary)
				, DriveCacheDeltaMixins::PatriciaTreeDelta(*driveSets.pPrimary, driveSets.pPatriciaTree)
				, DriveCacheDeltaMixins::BasicInsertRemove(*driveSets.pPrimary)
				, DriveCacheDeltaMixins::DeltaElements(*driveSets.pPrimary)
				, m_pDriveEntries(driveSets.pPrimary)
				, m_pMarkedDrivesAtHeight(driveSets.pHeightGrouping)
				, m_pConfigHolder(pConfigHolder)
		{}

	public:
		using DriveCacheDeltaMixins::ConstAccessor::find;
		using DriveCacheDeltaMixins::MutableAccessor::find;

		bool enabled() const {
			return ServicePluginEnabled(m_pConfigHolder, height());
		}

		void markDrive(const DriveCacheDescriptor::KeyType& key, const Height& height) {
			AddIdentifierWithGroup(*m_pMarkedDrivesAtHeight, height, key);
		}

		void unmarkDrive(const DriveCacheDescriptor::KeyType& key, const Height& height) {
			RemoveIdentifierWithGroup(*m_pMarkedDrivesAtHeight, height, key);
		}

        /// Processes all marked drives
        void processMarkedDrives(Height height, const consumer<DriveCacheDescriptor::ValueType&>& consumer) {
            ForEachIdentifierWithGroup(*m_pDriveEntries, *m_pMarkedDrivesAtHeight, height, consumer);
        }

        void prune(Height height) {
//            ForEachIdentifierWithGroup(
//                *m_pDriveEntries,
//                *m_pMarkedDrivesAtHeight,
//                height,
//                [this, height,](auto& history) {
//                    auto originalSizes = GetNamespaceSizes(history);
//                    auto removedIds = history.prune(height);
//                    auto newSizes = GetNamespaceSizes(history);
//
//                    collectedIds.insert(removedIds.cbegin(), removedIds.cend());
//                    for (auto removedId : removedIds)
//                        m_pNamespaceById->remove(removedId);
//
//                    if (history.empty())
//                        m_pHistoryById->remove(history.id());
//
//                    decrementActiveSize(originalSizes.Active - newSizes.Active);
//                    decrementDeepSize(originalSizes.Deep - newSizes.Deep);
//                });
            m_pMarkedDrivesAtHeight->remove(height);
        }

	private:
		DriveCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pDriveEntries;
		DriveCacheTypes::HeightGroupingTypes::BaseSetDeltaPointerType m_pMarkedDrivesAtHeight;
		std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
	};

	/// Delta on top of the drive cache.
	class DriveCacheDelta : public ReadOnlyViewSupplier<BasicDriveCacheDelta> {
	public:
		/// Creates a delta around \a driveSets and \a pConfigHolder.
		explicit DriveCacheDelta(
			const DriveCacheTypes::BaseSetDeltaPointers& driveSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(driveSets, pConfigHolder)
		{}
	};
}}
