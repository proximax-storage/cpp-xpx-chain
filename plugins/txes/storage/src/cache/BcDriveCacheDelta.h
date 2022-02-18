/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DriveKeyCollector.h"
#include "BcDriveBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"
#include "src/config/StorageConfiguration.h"

namespace catapult { namespace cache {

	/// Mixins used by the drive cache delta.
	using BcDriveCacheDeltaMixins = PatriciaTreeCacheMixins<BcDriveCacheTypes::PrimaryTypes::BaseSetDeltaType, BcDriveCacheDescriptor>;

	/// Basic delta on top of the drive cache.
	class BasicBcDriveCacheDelta
			: public utils::MoveOnly
			, public BcDriveCacheDeltaMixins::Size
			, public BcDriveCacheDeltaMixins::Contains
			, public BcDriveCacheDeltaMixins::ConstAccessor
			, public BcDriveCacheDeltaMixins::MutableAccessor
			, public BcDriveCacheDeltaMixins::PatriciaTreeDelta
			, public BcDriveCacheDeltaMixins::BasicInsertRemove
			, public BcDriveCacheDeltaMixins::DeltaElements
			, public BcDriveCacheDeltaMixins::ConfigBasedEnable<config::StorageConfiguration> {
	public:
		using ReadOnlyView = BcDriveCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a driveSets.
		explicit BasicBcDriveCacheDelta(
			const BcDriveCacheTypes::BaseSetDeltaPointers& driveSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: BcDriveCacheDeltaMixins::Size(*driveSets.pPrimary)
				, BcDriveCacheDeltaMixins::Contains(*driveSets.pPrimary)
				, BcDriveCacheDeltaMixins::ConstAccessor(*driveSets.pPrimary)
				, BcDriveCacheDeltaMixins::MutableAccessor(*driveSets.pPrimary)
				, BcDriveCacheDeltaMixins::PatriciaTreeDelta(*driveSets.pPrimary, driveSets.pPatriciaTree)
				, BcDriveCacheDeltaMixins::BasicInsertRemove(*driveSets.pPrimary)
				, BcDriveCacheDeltaMixins::DeltaElements(*driveSets.pPrimary)
				, BcDriveCacheDeltaMixins::ConfigBasedEnable<config::StorageConfiguration>(
					pConfigHolder, [](const auto& config) { return config.Enabled; })
				, m_pBcDriveEntries(driveSets.pPrimary)
				, m_pDeltaKeys(driveSets.pDeltaKeys)
				, m_PrimaryKeys(driveSets.PrimaryKeys)
		{}

	public:
		using BcDriveCacheDeltaMixins::ConstAccessor::find;
		using BcDriveCacheDeltaMixins::MutableAccessor::find;

	public:
		/// Inserts the drive \a entry into the cache.
		void insert(const state::BcDriveEntry& entry) {
			BcDriveCacheDeltaMixins::BasicInsertRemove::insert(entry);
			insertKey(entry.key());
		}

		/// Inserts the \a key into the cache.
		void insertKey(const Key& key) {
			if (!m_pDeltaKeys->contains(key))
				m_pDeltaKeys->insert(key);
		}

		/// Removes the drive \a entry into the cache.
		void remove(const Key& key) {
			BcDriveCacheDeltaMixins::BasicInsertRemove::remove(key);
			if (m_pDeltaKeys->contains(key))
				m_pDeltaKeys->remove(key);
		}

		/// Returns keys available after commit
		std::set<Key> keys() const {
			std::set<Key> result;
			for (const auto& key : deltaset::MakeIterableView(m_PrimaryKeys)) {
				result.insert(key);
			}

			auto deltas = m_pDeltaKeys->deltas();

			for (const auto& key : deltas.Added) {
				result.insert(key);
			}

			for (const auto& key : deltas.Removed) {
				result.erase(key);
			}

			return result;
		}

		void updateKeyCollector(const std::shared_ptr<DriveKeyCollector>& pKeyCollector) const {
			pKeyCollector->keys().clear();
			for (const auto& key : keys()) {
				auto iter = m_pBcDriveEntries->find(key);
				auto pEntry = iter.get();
				pKeyCollector->addKey(*pEntry);
			}
		}

		auto pruningBoundary() const {
			return deltaset::PruningBoundary<Key>();
		}

	private:
		BcDriveCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pBcDriveEntries;
		BcDriveCacheTypes::KeyTypes::BaseSetDeltaPointerType m_pDeltaKeys;
		const BcDriveCacheTypes::KeyTypes::BaseSetType& m_PrimaryKeys;
	};

	/// Delta on top of the drive cache.
	class BcDriveCacheDelta : public ReadOnlyViewSupplier<BasicBcDriveCacheDelta> {
	public:
		/// Creates a delta around \a driveSets.
		explicit BcDriveCacheDelta(
			const BcDriveCacheTypes::BaseSetDeltaPointers& driveSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(driveSets, pConfigHolder)
		{}
	};
}}
