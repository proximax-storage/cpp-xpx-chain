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

#include "Observers.h"
#include "catapult/cache_core/AccountStateCache.h"

namespace catapult { namespace observers {

	namespace {
		void Transfer(state::AccountState& debitState, state::AccountState& creditState, MosaicId mosaicId, Amount amount, Height height) {
			debitState.Balances.debit(mosaicId, amount, height);
			creditState.Balances.credit(mosaicId, amount, height);
		}
	}

	DEFINE_OBSERVER(BalanceTransfer, model::BalanceTransferNotification<1>, [](const auto& notification, const ObserverContext& context) {
		auto& cache = context.Cache.sub<cache::AccountStateCache>();
		auto senderIter = cache.find(notification.Sender);
		auto recipientIter = cache.find(context.Resolvers.resolve(notification.Recipient));

		auto& senderState = senderIter.get();
		auto& recipientState = recipientIter.get();

		auto mosaicId = context.Resolvers.resolve(notification.MosaicId);
		auto amount = context.Resolvers.resolve(notification.Amount);
		if (NotifyMode::Commit == context.Mode)
			Transfer(senderState, recipientState, mosaicId, amount, context.Height);
		else
			Transfer(recipientState, senderState, mosaicId, amount, context.Height);
	});
}}
