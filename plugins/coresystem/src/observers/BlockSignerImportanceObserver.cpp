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

#include "catapult/cache_core/AccountStateCacheUtils.h"
#include "Observers.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/state/BlockDifficultyInfo.h"

namespace catapult { namespace observers {

	namespace {
		using Notification = model::BlockSignerImportanceNotification<1>;
		void ObserveNotification(const Notification& notification, ObserverContext& context, const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder)
		{
			auto& cache = context.Cache.sub<cache::AccountStateCache>();
			if (NotifyMode::Commit == context.Mode)
			{
				auto accountStateOpt = cache::FindAccountStateByPublicKeyOrAddress(cache.asReadOnly(), notification.Signer);
				const auto& config = pConfigHolder->Config();
				auto balances = accountStateOpt->Balances.getCompoundEffectiveBalance(context.Height, config.Network.ImportanceGrouping);
				auto receipt = model::SignerBalanceReceipt(model::Receipt_Type_Block_Signer_Importance, balances.first, balances.second);
				context.StatementBuilder().addPublicKeyReceipt(receipt);
			}
		}
	}
	DECLARE_OBSERVER(BlockSignerImportance, Notification)(
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return MAKE_OBSERVER(BlockSignerImportance, Notification, ([pConfigHolder](const auto& notification, auto& context) {
		  ObserveNotification(notification, context, pConfigHolder);
		}));
	}
}}
