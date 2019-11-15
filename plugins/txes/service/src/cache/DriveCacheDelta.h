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
//#include "catapult/cache_core/AccountStateCache.h"
//#include "catapult/state/AccountState.h"
#include "src/observers/CommonDrive.h"
//#include "plugins/txes/multisig/src/cache/MultisigCache.h"
//#include "plugins/txes/multisig/src/observers/MultisigAccountFacade.h"
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
				, m_pBillingAtHeight(driveSets.pBillingGrouping)
				, m_pRemoveAtHeight(driveSets.pRemoveGrouping)
				, m_pConfigHolder(pConfigHolder)
		{}

	public:
		using DriveCacheDeltaMixins::ConstAccessor::find;
		using DriveCacheDeltaMixins::MutableAccessor::find;

		bool enabled() const {
			return ServicePluginEnabled(m_pConfigHolder, height());
		}

		void addBillingDrive(const DriveCacheDescriptor::KeyType& key, const Height& height) {
			AddIdentifierWithGroup(*m_pBillingAtHeight, height, key);
		}

		void removeBillingDrive(const DriveCacheDescriptor::KeyType& key, const Height& height) {
			RemoveIdentifierWithGroup(*m_pBillingAtHeight, height, key);
		}

        /// Processes all marked drives
        void processBillingDrives(Height height, const consumer<DriveCacheDescriptor::ValueType&>& consumer) {
            ForEachIdentifierWithGroup(*m_pDriveEntries, *m_pBillingAtHeight, height, consumer);
        }

		void markRemoveDrive(const DriveCacheDescriptor::KeyType& key, const Height& height) {
			AddIdentifierWithGroup(*m_pRemoveAtHeight, height, key);
		}

		void unmarkRemoveDrive(const DriveCacheDescriptor::KeyType& key, const Height& height) {
			RemoveIdentifierWithGroup(*m_pRemoveAtHeight, height, key);
		}

        void prune(Height height, observers::ObserverContext&) {
            ForEachIdentifierWithGroup(*m_pDriveEntries, *m_pRemoveAtHeight, height, [&](state::DriveEntry& driveEntry) {
                state::FilesMap& files = driveEntry.files();
                state::ReplicatorsMap& replicators = driveEntry.replicators();
                for (auto iter = files.begin(); iter != files.end();) {
                    if (iter->second.Deposit.unwrap() == 0 && iter->second.Payments.back().Height == height) {
                        for(auto& replicator : replicators) {
                            replicator.second.FilesWithoutDeposit.erase(iter->first);
                        }

                        iter = files.erase(iter);
                    } else {
                        ++iter;
                    }
                }

                if (files.empty() && driveEntry.state() >= state::DriveState::Finished && driveEntry.end().unwrap() == 0) {
                	// We can try to return remaining tokens to owner. but we need to ask researchers about that
//					auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
//					auto accountIter = accountStateCache.find(driveEntry.key());
//					state::AccountState& driveAccount = accountIter.get();
//
//					auto ownerIter = accountStateCache.find(driveEntry.owner());
//					state::AccountState& ownerAccount = ownerIter.get();
//
//					for (const auto& pair : driveAccount.Balances) {
//						observers::Transfer(driveAccount, ownerAccount, pair.first, pair.second, context.Height);
//					}
//
//					auto& multisigCache = context.Cache.sub<cache::MultisigCache>();
//					observers::MultisigAccountFacade multisigAccountFacade(multisigCache, driveEntry.key());
//					for (const auto& replicator : replicators())
//						multisigAccountFacade.removeCosignatory(replicator.first);
//
//					driveEntry.setEnd(context.Height);
//					markRemoveDrive(driveEntry.key(), context.Height);
					driveEntry.setEnd(height);
                }

                if (driveEntry.end() == height) {
					m_pDriveEntries->remove(driveEntry.key());
                }
            });
			m_pBillingAtHeight->remove(height);
			m_pRemoveAtHeight->remove(height);
        }

	private:
		DriveCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pDriveEntries;
		DriveCacheTypes::HeightGroupingTypes::BaseSetDeltaPointerType m_pBillingAtHeight;
		DriveCacheTypes::HeightGroupingTypes::BaseSetDeltaPointerType m_pRemoveAtHeight;
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
