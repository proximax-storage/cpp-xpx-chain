/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <utility>

#include "DbrbViewBaseSets.h"
#include "DbrbViewFetcherImpl.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"
#include "src/config/DbrbConfiguration.h"

namespace catapult { namespace cache {

	/// Mixins used by the DBRB view delta view.
	using DbrbViewCacheDeltaMixins = PatriciaTreeCacheMixins<DbrbViewCacheTypes::PrimaryTypes::BaseSetDeltaType, DbrbViewCacheDescriptor>;

	/// Basic delta on top of the DBRB view cache.
	class BasicDbrbViewCacheDelta
			: public utils::MoveOnly
			, public DbrbViewCacheDeltaMixins::Size
			, public DbrbViewCacheDeltaMixins::Contains
			, public DbrbViewCacheDeltaMixins::ConstAccessor
			, public DbrbViewCacheDeltaMixins::MutableAccessor
			, public DbrbViewCacheDeltaMixins::PatriciaTreeDelta
			, public DbrbViewCacheDeltaMixins::BasicInsertRemove
			, public DbrbViewCacheDeltaMixins::DeltaElements
			, public DbrbViewCacheDeltaMixins::ConfigBasedEnable<config::DbrbConfiguration> {
	public:
		using ReadOnlyView = DbrbViewCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a dbrbViewSets and \a pConfigHolder.
		explicit BasicDbrbViewCacheDelta(
			const DbrbViewCacheTypes::BaseSetDeltaPointers& dbrbViewSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: DbrbViewCacheDeltaMixins::Size(*dbrbViewSets.pPrimary)
				, DbrbViewCacheDeltaMixins::Contains(*dbrbViewSets.pPrimary)
				, DbrbViewCacheDeltaMixins::ConstAccessor(*dbrbViewSets.pPrimary)
				, DbrbViewCacheDeltaMixins::MutableAccessor(*dbrbViewSets.pPrimary)
				, DbrbViewCacheDeltaMixins::PatriciaTreeDelta(*dbrbViewSets.pPrimary, dbrbViewSets.pPatriciaTree)
				, DbrbViewCacheDeltaMixins::BasicInsertRemove(*dbrbViewSets.pPrimary)
				, DbrbViewCacheDeltaMixins::DeltaElements(*dbrbViewSets.pPrimary)
				, DbrbViewCacheDeltaMixins::ConfigBasedEnable<config::DbrbConfiguration>(std::move(pConfigHolder), [](const auto& config) { return config.Enabled; })
				, m_pDbrbProcessEntries(dbrbViewSets.pPrimary)
				, m_pDeltaProcessIds(dbrbViewSets.pDeltaProcessIds)
				, m_PrimaryProcessIds(dbrbViewSets.PrimaryProcessIds)
		{}

	public:
		using DbrbViewCacheDeltaMixins::ConstAccessor::find;
		using DbrbViewCacheDeltaMixins::MutableAccessor::find;

	public:
		void insert(const state::DbrbProcessEntry& entry) {
			DbrbViewCacheDeltaMixins::BasicInsertRemove::insert(entry);
			insertProcessId(entry.processId());
		}

		void insertProcessId(const dbrb::ProcessId& processId) {
			if (!m_pDeltaProcessIds->contains(processId))
				m_pDeltaProcessIds->insert(processId);
		}

		void remove(const dbrb::ProcessId& processId) {
			DbrbViewCacheDeltaMixins::BasicInsertRemove::remove(processId);
			if (m_pDeltaProcessIds->contains(processId))
				m_pDeltaProcessIds->remove(processId);
		}

		std::set<dbrb::ProcessId> processIds() const {
			std::set<dbrb::ProcessId> result;
			for (const auto& processId : deltaset::MakeIterableView(m_PrimaryProcessIds))
				result.insert(processId);

			auto deltas = m_pDeltaProcessIds->deltas();

			for (const auto& processId : deltas.Added)
				result.insert(processId);

			for (const auto& processId : deltas.Removed)
				result.erase(processId);

			return result;
		}

		void updateDbrbView(DbrbViewFetcherImpl& dbrbViewFetcher) const {
			dbrbViewFetcher.clear();
			for (const auto& processId : processIds()) {
				auto iter = m_pDbrbProcessEntries->find(processId);
				auto pEntry = iter.get();
				dbrbViewFetcher.addDbrbProcess(*pEntry);
			}
		}

	private:
		DbrbViewCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pDbrbProcessEntries;
		DbrbViewCacheTypes::ProcessIdTypes::BaseSetDeltaPointerType m_pDeltaProcessIds;
		const DbrbViewCacheTypes::ProcessIdTypes::BaseSetType& m_PrimaryProcessIds;
	};

	/// Delta on top of the DBRB view cache.
	class DbrbViewCacheDelta : public ReadOnlyViewSupplier<BasicDbrbViewCacheDelta> {
	public:
		/// Creates a delta around \a dbrbViewSets and \a pConfigHolder.
		explicit DbrbViewCacheDelta(
			const DbrbViewCacheTypes::BaseSetDeltaPointers& dbrbViewSets,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder)
				: ReadOnlyViewSupplier(dbrbViewSets, pConfigHolder)
		{}
	};
}}
