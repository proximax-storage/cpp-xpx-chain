/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ReplicatorKeyCollector.h"
#include "ReplicatorBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"
#include "src/config/StorageConfiguration.h"

namespace catapult { namespace cache {

	/// Mixins used by the replicator cache delta.
	using ReplicatorCacheDeltaMixins = PatriciaTreeCacheMixins<ReplicatorCacheTypes::PrimaryTypes::BaseSetDeltaType, ReplicatorCacheDescriptor>;

	/// Basic delta on top of the replicator cache.
	class BasicReplicatorCacheDelta
			: public utils::MoveOnly
			, public ReplicatorCacheDeltaMixins::Size
			, public ReplicatorCacheDeltaMixins::Contains
			, public ReplicatorCacheDeltaMixins::ConstAccessor
			, public ReplicatorCacheDeltaMixins::MutableAccessor
			, public ReplicatorCacheDeltaMixins::PatriciaTreeDelta
			, public ReplicatorCacheDeltaMixins::BasicInsertRemove
			, public ReplicatorCacheDeltaMixins::DeltaElements
			, public ReplicatorCacheDeltaMixins::ConfigBasedEnable<config::StorageConfiguration> {
	public:
		using ReadOnlyView = ReplicatorCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a replicatorSets.
		explicit BasicReplicatorCacheDelta(
			const ReplicatorCacheTypes::BaseSetDeltaPointers& replicatorSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReplicatorCacheDeltaMixins::Size(*replicatorSets.pPrimary)
				, ReplicatorCacheDeltaMixins::Contains(*replicatorSets.pPrimary)
				, ReplicatorCacheDeltaMixins::ConstAccessor(*replicatorSets.pPrimary)
				, ReplicatorCacheDeltaMixins::MutableAccessor(*replicatorSets.pPrimary)
				, ReplicatorCacheDeltaMixins::PatriciaTreeDelta(*replicatorSets.pPrimary, replicatorSets.pPatriciaTree)
				, ReplicatorCacheDeltaMixins::BasicInsertRemove(*replicatorSets.pPrimary)
				, ReplicatorCacheDeltaMixins::DeltaElements(*replicatorSets.pPrimary)
				, ReplicatorCacheDeltaMixins::ConfigBasedEnable<config::StorageConfiguration>(
					pConfigHolder, [](const auto& config) { return config.Enabled; })
				, m_pReplicatorEntries(replicatorSets.pPrimary)
				, m_pDeltaKeys(replicatorSets.pDeltaKeys)
				, m_PrimaryKeys(replicatorSets.PrimaryKeys)
		{}

	public:
		using ReplicatorCacheDeltaMixins::ConstAccessor::find;
		using ReplicatorCacheDeltaMixins::MutableAccessor::find;

	public:
		/// Inserts the replicator \a entry into the cache.
		void insert(const state::ReplicatorEntry& entry) {
			ReplicatorCacheDeltaMixins::BasicInsertRemove::insert(entry);
			insertKey(entry.key());
		}

		/// Inserts the \a key into the cache.
		void insertKey(const Key& key) {
			if (!m_pDeltaKeys->contains(key))
				m_pDeltaKeys->insert(key);
		}

		/// Removes the replicator \a entry into the cache.
		void remove(const Key& key) {
			ReplicatorCacheDeltaMixins::BasicInsertRemove::remove(key);
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

		void updateKeyCollector(const std::shared_ptr<ReplicatorKeyCollector>& pKeyCollector) const {
			pKeyCollector->keys().clear();
			for (const auto& key : keys()) {
				auto iter = m_pReplicatorEntries->find(key);
				auto pEntry = iter.get();
				pKeyCollector->addKey(*pEntry);
			}
		}

	private:
		ReplicatorCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pReplicatorEntries;
		ReplicatorCacheTypes::KeyTypes::BaseSetDeltaPointerType m_pDeltaKeys;
		const ReplicatorCacheTypes::KeyTypes::BaseSetType& m_PrimaryKeys;
	};

	/// Delta on top of the replicator cache.
	class ReplicatorCacheDelta : public ReadOnlyViewSupplier<BasicReplicatorCacheDelta> {
	public:
		/// Creates a delta around \a replicatorSets.
		explicit ReplicatorCacheDelta(
			const ReplicatorCacheTypes::BaseSetDeltaPointers& replicatorSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(replicatorSets, pConfigHolder)
		{}
	};
}}
