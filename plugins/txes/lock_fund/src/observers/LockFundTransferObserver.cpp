/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "src/cache/LockFundCache.h"
#include "Observers.h"
#include "src/config/LockFundConfiguration.h"
#include "catapult/cache_core/AccountStateCache.h"

namespace catapult { namespace observers {

	namespace {
		template<model::LockFundAction TLockAction>
		void Lock(state::AccountState& accountState, cache::LockFundCacheDelta& lockFundCache, const std::map<MosaicId, Amount>& mosaics, Height height, BlockDuration duration) {
			if constexpr(TLockAction == model::LockFundAction::Lock)
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
		template<model::LockFundAction TLockAction>
		void Unlock(state::AccountState& accountState, cache::LockFundCacheDelta& lockFundCache, const config::LockFundConfiguration& pluginConfig, const std::map<MosaicId, Amount>& mosaics, Height unlockHeight) {
			if constexpr(TLockAction == model::LockFundAction::Unlock)
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
					Lock<model::LockFundAction::Lock>(senderState, lockFundCache, mosaics, context.Height, notification.Duration);
				else
					Lock<model::LockFundAction::Unlock>(senderState, lockFundCache, mosaics, context.Height, notification.Duration);
			}
			else
			{
				auto targetHeight = notification.Duration.unwrap() + context.Height.unwrap();
				if(notification.Duration == BlockDuration(0))
					targetHeight += pluginConfig.MinRequestUnlockCooldown.unwrap();
				if (NotifyMode::Commit == context.Mode)
					Unlock<model::LockFundAction::Unlock>(senderState, lockFundCache, pluginConfig, mosaics, Height(targetHeight));
				else
					Unlock<model::LockFundAction::Lock>(senderState, lockFundCache, pluginConfig, mosaics, Height(targetHeight));
			}

		}
	}

	DEFINE_OBSERVER(LockFundTransfer, model::LockFundTransferNotification<1>, ObserveAndValidate);
}}
