/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/LockFundCache.h"
#include "Observers.h"
#include "src/config/LockFundConfiguration.h"
#include "catapult/cache_core/AccountStateCache.h"

namespace catapult { namespace observers {

	namespace {
		template<NotifyMode TLockAction>
		void Lock(state::AccountState& accountState, cache::LockFundCacheDelta& lockFundCache, const std::map<MosaicId, Amount>& mosaics, Height height, BlockDuration duration) {
			if constexpr(TLockAction == NotifyMode::Commit)
			{
				for(auto mosaic : mosaics)
					accountState.Balances.lock(mosaic.first, mosaic.second, height);
				if(duration != BlockDuration(0))
					lockFundCache.insert(accountState.PublicKey, Height(height.unwrap()+duration.unwrap()), mosaics);
			}
			else
			{
				for(auto mosaic : mosaics)
					accountState.Balances.unlock(mosaic.first, mosaic.second, height);
				if(duration != BlockDuration(0))
					lockFundCache.remove(accountState.PublicKey, Height(height.unwrap()+duration.unwrap()));
			}
		}
		template<NotifyMode TLockAction>
		void Unlock(state::AccountState& accountState, cache::LockFundCacheDelta& lockFundCache, const config::LockFundConfiguration& pluginConfig, const std::map<MosaicId, Amount>& mosaics, Height unlockHeight) {
			if constexpr(TLockAction == NotifyMode::Commit)
				lockFundCache.insert(accountState.PublicKey, unlockHeight, mosaics);
			else
				lockFundCache.remove(accountState.PublicKey, unlockHeight);
		}
		void ObserveAndValidate(const model::LockFundTransferNotification<1>& notification, const ObserverContext& context)
		{
			auto& cache = context.Cache.sub<cache::AccountStateCache>();
			auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::LockFundConfiguration>();
			auto senderIter = cache.find(notification.Sender);
			auto& senderState = senderIter.get();
			std::map<MosaicId, Amount> mosaics;
			for (auto& mosaic : notification.Mosaics)
				mosaics.insert(std::make_pair(context.Resolvers.resolve(mosaic.MosaicId), mosaic.Amount));
			auto& lockFundCache = context.Cache.sub<cache::LockFundCache>();

			if(notification.Action == model::LockFundAction::Lock)
			{
				if (NotifyMode::Commit == context.Mode)
					Lock<NotifyMode::Commit>(senderState, lockFundCache, mosaics, context.Height, notification.Duration);
				else
					Lock<NotifyMode::Rollback>(senderState, lockFundCache, mosaics, context.Height, notification.Duration);
			}
			else
			{
				auto targetHeight = notification.Duration.unwrap() + context.Height.unwrap();
				if(notification.Duration == BlockDuration(0))
					targetHeight += pluginConfig.MinRequestUnlockCooldown.unwrap();
				if (NotifyMode::Commit == context.Mode)
					Unlock<NotifyMode::Commit>(senderState, lockFundCache, pluginConfig, mosaics, Height(targetHeight));
				else
					Unlock<NotifyMode::Rollback>(senderState, lockFundCache, pluginConfig, mosaics, Height(targetHeight));
			}

		}
	}

	DEFINE_OBSERVER(LockFundTransfer, model::LockFundTransferNotification<1>, ObserveAndValidate);
}}
