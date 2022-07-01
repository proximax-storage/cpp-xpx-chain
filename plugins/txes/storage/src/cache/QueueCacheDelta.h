/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "QueueBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"
#include "src/config/StorageConfiguration.h"

namespace catapult { namespace cache {

	/// Mixins used by the drive cache delta.
	using QueueCacheDeltaMixins = PatriciaTreeCacheMixins<QueueCacheTypes::PrimaryTypes::BaseSetDeltaType, QueueCacheDescriptor>;

	/// Basic delta on top of the drive cache.
	class BasicQueueCacheDelta
			: public utils::MoveOnly
			, public QueueCacheDeltaMixins::Size
			, public QueueCacheDeltaMixins::Contains
			, public QueueCacheDeltaMixins::ConstAccessor
			, public QueueCacheDeltaMixins::MutableAccessor
			, public QueueCacheDeltaMixins::PatriciaTreeDelta
			, public QueueCacheDeltaMixins::BasicInsertRemove
			, public QueueCacheDeltaMixins::DeltaElements
			, public QueueCacheDeltaMixins::ConfigBasedEnable<config::StorageConfiguration> {
	public:
		using ReadOnlyView = QueueCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a driveSets.
		explicit BasicQueueCacheDelta(
			const QueueCacheTypes::BaseSetDeltaPointers& driveSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: QueueCacheDeltaMixins::Size(*driveSets.pPrimary)
				, QueueCacheDeltaMixins::Contains(*driveSets.pPrimary)
				, QueueCacheDeltaMixins::ConstAccessor(*driveSets.pPrimary)
				, QueueCacheDeltaMixins::MutableAccessor(*driveSets.pPrimary)
				, QueueCacheDeltaMixins::PatriciaTreeDelta(*driveSets.pPrimary, driveSets.pPatriciaTree)
				, QueueCacheDeltaMixins::BasicInsertRemove(*driveSets.pPrimary)
				, QueueCacheDeltaMixins::DeltaElements(*driveSets.pPrimary)
				, QueueCacheDeltaMixins::ConfigBasedEnable<config::StorageConfiguration>(
					pConfigHolder, [](const auto& config) { return config.Enabled; })
				, m_pQueueEntries(driveSets.pPrimary)
				, m_pDeltaKeys(driveSets.pDeltaKeys)
				, m_PrimaryKeys(driveSets.PrimaryKeys)
		{}

	public:
		using QueueCacheDeltaMixins::ConstAccessor::find;
		using QueueCacheDeltaMixins::MutableAccessor::find;

	public:
		/// Inserts the drive \a entry into the cache.
		void insert(const state::QueueEntry& entry) {
			QueueCacheDeltaMixins::BasicInsertRemove::insert(entry);
			insertKey(entry.key());
		}

		/// Inserts the \a key into the cache.
		void insertKey(const Key& key) {
			if (!m_pDeltaKeys->contains(key))
				m_pDeltaKeys->insert(key);
		}

		/// Removes the key \a entry from the cache.
		void remove(const Key& key) {
			QueueCacheDeltaMixins::BasicInsertRemove::remove(key);
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

	private:
		QueueCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pQueueEntries;
		QueueCacheTypes::KeyTypes::BaseSetDeltaPointerType m_pDeltaKeys;
		const QueueCacheTypes::KeyTypes::BaseSetType& m_PrimaryKeys;
	};

	/// Delta on top of the drive cache.
	class QueueCacheDelta : public ReadOnlyViewSupplier<BasicQueueCacheDelta> {
	public:
		/// Creates a delta around \a driveSets.
		explicit QueueCacheDelta(
			const QueueCacheTypes::BaseSetDeltaPointers& driveSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(driveSets, pConfigHolder)
		{}
	};
}}
