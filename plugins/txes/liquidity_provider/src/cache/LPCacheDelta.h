/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "LPBaseSets.h"
#include "src/config/LiquidityProviderConfiguration.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {

	/// Mixins used by the drive cache delta.
	using LPCacheDeltaMixins = PatriciaTreeCacheMixins<LPCacheTypes::PrimaryTypes::BaseSetDeltaType, LPCacheDescriptor>;

	/// Basic delta on top of the drive cache.
	class BasicLPCacheDelta
			: public utils::MoveOnly
			, public LPCacheDeltaMixins::Size
			, public LPCacheDeltaMixins::Contains
			, public LPCacheDeltaMixins::ConstAccessor
			, public LPCacheDeltaMixins::MutableAccessor
			, public LPCacheDeltaMixins::PatriciaTreeDelta
			, public LPCacheDeltaMixins::BasicInsertRemove
			, public LPCacheDeltaMixins::DeltaElements
			, public LPCacheDeltaMixins::ConfigBasedEnable<config::LiquidityProviderConfiguration> {
	public:
		using ReadOnlyView = LPCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a driveSets.
		explicit BasicLPCacheDelta(
			const LPCacheTypes::BaseSetDeltaPointers& driveSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: LPCacheDeltaMixins::Size(*driveSets.pPrimary)
				, LPCacheDeltaMixins::Contains(*driveSets.pPrimary)
				, LPCacheDeltaMixins::ConstAccessor(*driveSets.pPrimary)
				, LPCacheDeltaMixins::MutableAccessor(*driveSets.pPrimary)
				, LPCacheDeltaMixins::PatriciaTreeDelta(*driveSets.pPrimary, driveSets.pPatriciaTree)
				, LPCacheDeltaMixins::BasicInsertRemove(*driveSets.pPrimary)
				, LPCacheDeltaMixins::DeltaElements(*driveSets.pPrimary)
				, LPCacheDeltaMixins::ConfigBasedEnable<config::LiquidityProviderConfiguration>(
					pConfigHolder, [](const auto& config) { return config.Enabled; })
				, m_pLPEntries(driveSets.pPrimary)
				, m_pDeltaKeys(driveSets.pDeltaKeys)
				, m_PrimaryKeys(driveSets.PrimaryKeys)
		{}

	public:
		using LPCacheDeltaMixins::ConstAccessor::find;
		using LPCacheDeltaMixins::MutableAccessor::find;

	public:
		/// Inserts the drive \a entry into the cache.
		void insert(const state::LiquidityProviderEntry& entry) {
			LPCacheDeltaMixins::BasicInsertRemove::insert(entry);
			insertKey(entry.mosaicId());
		}

		/// Inserts the \a key into the cache.
		void insertKey(const MosaicId& mosaicId) {
			if (!m_pDeltaKeys->contains(mosaicId))
				m_pDeltaKeys->insert(mosaicId);
		}

		/// Removes the drive \a entry into the cache.
		void remove(const MosaicId& mosaicId) {
			LPCacheDeltaMixins::BasicInsertRemove::remove(mosaicId);
			if (m_pDeltaKeys->contains(mosaicId))
				m_pDeltaKeys->remove(mosaicId);
		}

		/// Returns keys available after commit
		std::set<MosaicId> keys() const {
			std::set<MosaicId> result;
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
		LPCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pLPEntries;
		LPCacheTypes::KeyTypes::BaseSetDeltaPointerType m_pDeltaKeys;
		const LPCacheTypes::KeyTypes::BaseSetType& m_PrimaryKeys;
	};

	/// Delta on top of the drive cache.
	class LPCacheDelta : public ReadOnlyViewSupplier<BasicLPCacheDelta> {
	public:
		/// Creates a delta around \a driveSets.
		explicit LPCacheDelta(
			const LPCacheTypes::BaseSetDeltaPointers& driveSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(driveSets, pConfigHolder)
		{}
	};
}}
