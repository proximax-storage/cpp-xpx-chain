/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <src/cache/LockFundCache.h>
#include <catapult/cache_core/AccountStateCache.h>
#include "Validators.h"
#include "src/config/LockFundConfiguration.h"

namespace catapult { namespace validators {

	using Notification = model::LockFundTransferNotification<1>;


	namespace {

		template<model::LockFundAction TAction>
		ValidationResult ValidateCanOperateOnFunds(const Notification& notification, const ValidatorContext& context, const state::AccountState& account, const config::LockFundConfiguration& pluginConfig)
		{
			UnresolvedMosaicId lastMosaicId;
			/// Container to keep track of funds available to unlock and the amounts to unlock.
			std::unordered_map<uint64_t, std::pair<Amount, Amount>> funds;
			for (auto i = 0u; i < notification.Mosaics.size(); ++i) {
				auto currentMosaicId = notification.Mosaics[i].MosaicId;
				if (i != 0 && lastMosaicId >= currentMosaicId)
					return Failure_LockFund_Out_Of_Order_Mosaics;

				if (1 > notification.Mosaics[i].Amount.unwrap())
					return Failure_LockFund_Zero_Amount;

				lastMosaicId = currentMosaicId;
				auto resolvedMosaic = context.Resolvers.resolve(currentMosaicId);
				if constexpr(TAction == model::LockFundAction::Lock)
					funds[resolvedMosaic.unwrap()] = std::make_pair(account.Balances.get(resolvedMosaic), notification.Mosaics[i].Amount);
				else
					funds[resolvedMosaic.unwrap()] = std::make_pair(account.Balances.getLocked(resolvedMosaic), notification.Mosaics[i].Amount);
			}
			if constexpr( TAction == model::LockFundAction::Lock)
			{
				if(notification.Duration != BlockDuration(0))
				{
					//Move this to own function to avoid duplication(temp)
					const auto& lockFundCache = context.Cache.template sub<cache::LockFundCache>();
					auto unlockingFundsRecordIt = lockFundCache.find(notification.Sender);
					auto unlockingFundsRecord = unlockingFundsRecordIt.tryGet();
					if(unlockingFundsRecord)
					{
						for(auto record : unlockingFundsRecord->LockFundRecords)
						{
							if(Height(context.Height.unwrap() + notification.Duration.unwrap()) == record.first && record.second.Active())
								return Failure_LockFund_Duplicate_Record;
						}
					}
				}
			}
			else
			{
				uint64_t targetHeight = context.Height.unwrap() + notification.Duration.unwrap();
				if(notification.Duration == BlockDuration(0))
					targetHeight += pluginConfig.MinRequestUnlockCooldown.unwrap();

				const auto& lockFundCache = context.Cache.template sub<cache::LockFundCache>();
				auto unlockingFundsRecordIt = lockFundCache.find(notification.Sender);
				auto unlockingFundsRecord = unlockingFundsRecordIt.tryGet();
				if(unlockingFundsRecord)
				{
					for(auto record : unlockingFundsRecord->LockFundRecords)
					{
						if(record.second.Active())
						{
							if(targetHeight == record.first.unwrap())
								return Failure_LockFund_Duplicate_Record;
							for(auto mosaic : funds)
							{
								auto mosaicRecord = record.second.Get().find(MosaicId(mosaic.first));
								if(mosaicRecord != record.second.Get().end())
								{
									funds[mosaic.first].first = funds[mosaic.first].first - mosaicRecord->second;
								}
							}
						}
					}
				}
			}
			for(auto resolvedPair : funds)
			{
				if(resolvedPair.second.second > resolvedPair.second.first)
					return Failure_LockFund_Not_Enough_Funds;
			}
			return ValidationResult::Success;
		}
	}
	DECLARE_STATEFUL_VALIDATOR(LockFundTransfer, Notification)() {
		return MAKE_STATEFUL_VALIDATOR(LockFundTransfer, ([](const Notification& notification, const ValidatorContext& context) {
            const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::LockFundConfiguration>();
			if(notification.Duration != BlockDuration(0) && notification.Duration < pluginConfig.MinRequestUnlockCooldown)
				return Failure_LockFund_Duration_Smaller_Than_Configured;

			if (1 > notification.Mosaics.size())
				return Failure_LockFund_Zero_Amount;

			if (pluginConfig.MaxMosaicsSize < notification.Mosaics.size())
				return Failure_LockFund_Too_Many_Mosaics;

			const auto& accountStateCache = context.Cache.template sub<cache::AccountStateCache>();
			auto accountIt = accountStateCache.find(notification.Sender);
			auto account = accountIt.tryGet();
			//Cannot lock/unlock funds unless account is a V2, existing, non locked account.
			if(!account || account->IsLocked() || account->GetVersion() < 2)
			{
				return Failure_LockFund_Invalid_Sender;
			}

			if(notification.Action == model::LockFundAction::Lock)
				return ValidateCanOperateOnFunds<model::LockFundAction::Lock>(notification, context, *account, pluginConfig);
			return ValidateCanOperateOnFunds<model::LockFundAction::Unlock>(notification, context, *account, pluginConfig);
		}));
	}
}}
