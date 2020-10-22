/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CommitteeAccountCollector.h"
#include "CommitteeCacheTypes.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"
#include "src/config/CommitteeConfiguration.h"

namespace catapult { namespace cache {

	/// Mixins used by the network config cache delta.
	using CommitteeCacheDeltaMixins = BasicCacheMixins<CommitteeCacheTypes::PrimaryTypes::BaseSetDeltaType, CommitteeCacheDescriptor>;

	/// Basic delta on top of the network config cache.
	class BasicCommitteeCacheDelta
			: public utils::MoveOnly
			, public CommitteeCacheDeltaMixins::Size
			, public CommitteeCacheDeltaMixins::Contains
			, public CommitteeCacheDeltaMixins::ConstAccessor
			, public CommitteeCacheDeltaMixins::MutableAccessor
			, public CommitteeCacheDeltaMixins::BasicInsertRemove
			, public CommitteeCacheDeltaMixins::DeltaElements
			, public CommitteeCacheDeltaMixins::ConfigBasedEnable<config::CommitteeConfiguration> {
	public:
		using ReadOnlyView = CommitteeCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a committeeSets.
		explicit BasicCommitteeCacheDelta(
			const CommitteeCacheTypes::BaseSetDeltaPointers& committeeSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: CommitteeCacheDeltaMixins::Size(*committeeSets.pPrimary)
				, CommitteeCacheDeltaMixins::Contains(*committeeSets.pPrimary)
				, CommitteeCacheDeltaMixins::ConstAccessor(*committeeSets.pPrimary)
				, CommitteeCacheDeltaMixins::MutableAccessor(*committeeSets.pPrimary)
				, CommitteeCacheDeltaMixins::BasicInsertRemove(*committeeSets.pPrimary)
				, CommitteeCacheDeltaMixins::DeltaElements(*committeeSets.pPrimary)
				, CommitteeCacheDeltaMixins::ConfigBasedEnable<config::CommitteeConfiguration>(
					pConfigHolder, [](const auto& config) { return config.Enabled; })
				, m_pCommitteeEntries(committeeSets.pPrimary)
		{}

	public:
		using CommitteeCacheDeltaMixins::ConstAccessor::find;
		using CommitteeCacheDeltaMixins::MutableAccessor::find;

	public:
		void updateAccountCollector(const std::shared_ptr<CommitteeAccountCollector>& pAccountCollector) const {
			auto deltas = m_pCommitteeEntries->deltas();
			auto added = deltas.Added;
			added.insert(deltas.Copied.begin(), deltas.Copied.end());
			auto removed = deltas.Removed;

			for (const auto& pair : added) {
				if (removed.find(pair.first) != removed.end()) {
					removed.erase(pair.first);
				} else {
					pAccountCollector->addAccount(pair.second);
				}
			}

			for (const auto& pair : removed) {
				pAccountCollector->removeAccount(pair.first);
			}
		}

	private:
		CommitteeCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pCommitteeEntries;
	};

	/// Delta on top of the committee cache.
	class CommitteeCacheDelta : public ReadOnlyViewSupplier<BasicCommitteeCacheDelta> {
	public:
		/// Creates a delta around \a committeeSets.
		explicit CommitteeCacheDelta(
			const CommitteeCacheTypes::BaseSetDeltaPointers& committeeSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(committeeSets, pConfigHolder)
		{}
	};
}}
