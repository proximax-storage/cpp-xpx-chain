/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ViewSequenceBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"
#include "src/config/DbrbConfiguration.h"

namespace catapult { namespace cache {

	/// Mixins used by the view sequence delta view.
	struct ViewSequenceCacheDeltaMixins {
	private:
		using PrimaryMixins = PatriciaTreeCacheMixins<
				ViewSequenceCacheTypes::PrimaryTypes::BaseSetDeltaType, ViewSequenceCacheDescriptor>;

	public:
		using Size = PrimaryMixins::Size;
		using Contains = PrimaryMixins::Contains;
		using PatriciaTreeDelta = PrimaryMixins::PatriciaTreeDelta;
		using MutableAccessor = PrimaryMixins::ConstAccessor;
		using ConstAccessor = PrimaryMixins::MutableAccessor;
		using DeltaElements = PrimaryMixins::DeltaElements;
		using BasicInsertRemove = PrimaryMixins::BasicInsertRemove;
		using ConfigBasedEnable = PrimaryMixins::ConfigBasedEnable<config::DbrbConfiguration>;
	};

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
			, public ViewSequenceCacheDeltaMixins::ConfigBasedEnable {
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
				, ViewSequenceCacheDeltaMixins::ConfigBasedEnable(pConfigHolder, [](const auto& config) { return config.Enabled; })
				, m_pViewSequenceEntries(viewSequenceSets.pPrimary)
				, m_pMessageHash(viewSequenceSets.pMessageHash)
		{}

	public:
		using ViewSequenceCacheDeltaMixins::ConstAccessor::find;
		using ViewSequenceCacheDeltaMixins::MutableAccessor::find;

		void addBillingViewSequence(const ViewSequenceCacheDescriptor::KeyType& key, const Height& height) {
			AddIdentifierWithGroup(*m_pBillingAtHeight, height, key);
		}

		void removeBillingViewSequence(const ViewSequenceCacheDescriptor::KeyType& key, const Height& height) {
			RemoveIdentifierWithGroup(*m_pBillingAtHeight, height, key);
		}

        /// Processes all marked view sequences
        void processBillingViewSequences(Height height, const consumer<ViewSequenceCacheDescriptor::ValueType&>& consumer) {
            ForEachIdentifierWithGroup(*m_pViewSequenceEntries, *m_pBillingAtHeight, height, consumer);
        }

		void markRemoveViewSequence(const ViewSequenceCacheDescriptor::KeyType& key, const Height& height) {
			AddIdentifierWithGroup(*m_pRemoveAtHeight, height, key);
		}

		void unmarkRemoveViewSequence(const ViewSequenceCacheDescriptor::KeyType& key, const Height& height) {
			RemoveIdentifierWithGroup(*m_pRemoveAtHeight, height, key);
		}

        void prune(Height height, observers::ObserverContext&) {
            ForEachIdentifierWithGroup(*m_pViewSequenceEntries, *m_pRemoveAtHeight, height, [&](state::ViewSequenceEntry& viewSequenceEntry) {
                if (viewSequenceEntry.end() == height && viewSequenceEntry.version() < 3) {
					m_pViewSequenceEntries->remove(viewSequenceEntry.key());
                }
            });
			m_pBillingAtHeight->remove(height);
			m_pRemoveAtHeight->remove(height);
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
