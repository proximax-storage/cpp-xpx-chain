/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/LockFundTotalStakedReceipt.h"
#include "plugins/services/globalstore/src/state/BaseConverters.h"
#include "plugins/services/globalstore/src/cache/GlobalStoreCache.h"
#include "src/cache/LockFundCache.h"
#include "Observers.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "src/model/LockFundReceiptType.h"

namespace catapult { namespace observers {

	namespace {
		void ProcessLockfundUpdate(const model::BlockNotification<1>& notification, ObserverContext& context)
		{
			const model::NetworkConfiguration& config = context.Config.Network;
			auto& lockFundCache = context.Cache.sub<cache::LockFundCache>();
			auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
			auto& globalStore = context.Cache.sub<cache::GlobalStoreCache>();
			auto totalStakedRecordIter = globalStore.find(config::TotalStaked_GlobalKey);
			auto& totalStakedRecord = totalStakedRecordIter.get();
			auto& totalStakedRefData = totalStakedRecord.GetRef<state::Uint64Converter>();
			if (context.Mode == NotifyMode::Commit)
			{
				auto activeHeightRecord = lockFundCache.find(context.Height);
				auto record = activeHeightRecord.tryGet();
				if(record)
				{
					// Unlock amounts
					for(auto& irecord : record->LockFundRecords)
					{
						if(irecord.second.Active())
						{
							auto accountIter = accountStateCache.find(irecord.first);
							auto &account = accountIter.get();
							for(auto& mosaic : irecord.second.Get())
							{
								if(mosaic.first == context.Config.Immutable.HarvestingMosaicId)
								{
									totalStakedRefData -= mosaic.second.unwrap();
								}
								account.Balances.unlock(mosaic.first, mosaic.second, context.Height);
							}
						}
					}
					// Disable records
					lockFundCache.disable(context.Height);
				}

				model::TotalStakedReceipt totalStakedReceipt(model::Receipt_Type_Total_Staked, Amount(totalStakedRefData));
				context.StatementBuilder().addBlockchainStateReceipt(totalStakedReceipt);
				//Clear aged records
				if (context.Height.unwrap() - (config.MaxRollbackBlocks*2) <= 0)
					return;
				auto clearHeight = context.Height-Height(config.MaxRollbackBlocks*2);
				auto heightRecord = lockFundCache.find(clearHeight);
				auto agedRecord = heightRecord.tryGet();
				if(agedRecord)
				{
					// Clear records
					lockFundCache.remove(clearHeight);
				}
			}
			else
			{
				auto activeHeightRecord = lockFundCache.find(context.Height);
				auto record = activeHeightRecord.tryGet();
				if(record)
				{
					// Disable records
					lockFundCache.recover(context.Height);
					// Unlock amounts
					for(auto& irecord : record->LockFundRecords)
					{
						auto accountIter = accountStateCache.find(irecord.first);
						auto& account = accountIter.get();
						if(irecord.second.Active()) //Redundant
						{
							for(auto& mosaic : irecord.second.Get())
							{
								if(mosaic.first == context.Config.Immutable.HarvestingMosaicId)
								{
									totalStakedRefData += mosaic.second.unwrap();
								}
								account.Balances.lock(mosaic.first, mosaic.second);
							}
						}
					}
				}
			}
		}
		void PluginSetup(const ObserverContext& context) {
			auto& globalStore = context.Cache.sub<cache::GlobalStoreCache>();
			auto recordIter = globalStore.find(config::LockFundPluginInstalled_GlobalKey);
			if (NotifyMode::Commit == context.Mode) {
				if(!recordIter.tryGet())
				{
					state::GlobalEntry installedRecord(config::LockFundPluginInstalled_GlobalKey, context.Height.unwrap(), state::PluginInstallConverter());
					state::GlobalEntry totalStaked(config::TotalStaked_GlobalKey, 0, state::Uint64Converter());
					globalStore.insert(installedRecord);
					globalStore.insert(totalStaked);

				}
			} else {
				if(recordIter.tryGet() && recordIter.get().Get<state::PluginInstallConverter>() == context.Height.unwrap())
				{
					globalStore.remove(config::LockFundPluginInstalled_GlobalKey);
					globalStore.remove(config::TotalStaked_GlobalKey);
				}
			}
		}
		void ObserveNotification(const model::BlockNotification<1>& notification, ObserverContext& context) {
			auto newConfig = context.Config.Network.GetPluginConfiguration<config::LockFundConfiguration>();
			if(!newConfig.Enabled) return;

			// This is the initial blockchain configuration and the plugin is enabled
			// or this is the activation height for a new configuration in which the plugin has just now become enabled
			if(context.HasChange(model::StateChangeFlags::Blockchain_Init) || (context.HasChange(model::StateChangeFlags::Network_Config_Upgraded) && !context.Config.PreviousConfiguration->Network.GetPluginConfiguration<config::LockFundConfiguration>().Enabled))
			{
				PluginSetup(context);
				return;
			}

			ProcessLockfundUpdate(notification, context);
		}
	}
	/// Note: Consider removing records in batches instead
	DEFINE_OBSERVER(LockFundBlock, model::BlockNotification<1>, ObserveNotification);
}}
