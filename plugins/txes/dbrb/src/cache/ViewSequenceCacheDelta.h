/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ViewSequenceBaseSets.h"
#include "ReadOnlyViewSequenceCache.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"
#include "src/config/DbrbConfiguration.h"

namespace catapult { namespace cache {

	/// Mixins used by the view sequence delta view.
	using ViewSequenceCacheDeltaMixins = PatriciaTreeCacheMixins<ViewSequenceCacheTypes::PrimaryTypes::BaseSetDeltaType, ViewSequenceCacheDescriptor>;

	/// Basic delta on top of the view sequence cache.
	class BasicViewSequenceCacheDelta
			: public utils::MoveOnly
			, public ViewSequenceCacheDeltaMixins::Size
			, public ViewSequenceCacheDeltaMixins::Contains
			, public ViewSequenceCacheDeltaMixins::ConstAccessor
			, public ViewSequenceCacheDeltaMixins::MutableAccessor
			, public ViewSequenceCacheDeltaMixins::PatriciaTreeDelta
			, public ViewSequenceCacheDeltaMixins::BasicInsertRemove
			, public ViewSequenceCacheDeltaMixins::DeltaElements
			, public ViewSequenceCacheDeltaMixins::ConfigBasedEnable<config::DbrbConfiguration> {
	public:
		using ReadOnlyView = ViewSequenceCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a viewSequenceSets and \a pConfigHolder.
		explicit BasicViewSequenceCacheDelta(
			const ViewSequenceCacheTypes::BaseSetDeltaPointers& viewSequenceSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ViewSequenceCacheDeltaMixins::Size(*viewSequenceSets.pPrimary)
				, ViewSequenceCacheDeltaMixins::Contains(*viewSequenceSets.pPrimary)
				, ViewSequenceCacheDeltaMixins::ConstAccessor(*viewSequenceSets.pPrimary)
				, ViewSequenceCacheDeltaMixins::MutableAccessor(*viewSequenceSets.pPrimary)
				, ViewSequenceCacheDeltaMixins::PatriciaTreeDelta(*viewSequenceSets.pPrimary, viewSequenceSets.pPatriciaTree)
				, ViewSequenceCacheDeltaMixins::BasicInsertRemove(*viewSequenceSets.pPrimary)
				, ViewSequenceCacheDeltaMixins::DeltaElements(*viewSequenceSets.pPrimary)
				, ViewSequenceCacheDeltaMixins::ConfigBasedEnable<config::DbrbConfiguration>(pConfigHolder, [](const auto& config) { return config.Enabled; })
				, m_pViewSequenceEntries(viewSequenceSets.pPrimary)
				, m_pMessageHash(viewSequenceSets.pMessageHash)
		{}

	public:
		using ViewSequenceCacheDeltaMixins::ConstAccessor::find;
		using ViewSequenceCacheDeltaMixins::MutableAccessor::find;

	public:
		/// Inserts the committee \a entry into the cache.
		void insert(const state::ViewSequenceEntry& entry) {
			ViewSequenceCacheDeltaMixins::BasicInsertRemove::insert(entry);

			auto iter = m_pMessageHash->find(0u);
			auto pEntry = iter.get();
			if (pEntry) {
				pEntry->setHash(entry.hash());
			} else {
				m_pMessageHash->insert(state::MessageHashEntry(0u, entry.hash()));
			}
		}

		dbrb::View getLatestView() const {
			if (m_pMessageHash->contains(0u)) {
				auto hash = m_pMessageHash->find(0u).get()->hash();
				return m_pViewSequenceEntries->find(hash).get()->mostRecentView();
			}

			return {};
		}

	private:
		ViewSequenceCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pViewSequenceEntries;
		ViewSequenceCacheTypes::MessageHashTypes::BaseSetDeltaPointerType m_pMessageHash;
	};

	/// Delta on top of the view sequence cache.
	class ViewSequenceCacheDelta : public ReadOnlyViewSupplier<BasicViewSequenceCacheDelta> {
	public:
		/// Creates a delta around \a viewSequenceSets and \a pConfigHolder.
		explicit ViewSequenceCacheDelta(
			const ViewSequenceCacheTypes::BaseSetDeltaPointers& viewSequenceSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(viewSequenceSets, pConfigHolder)
		{}
	};
}}
