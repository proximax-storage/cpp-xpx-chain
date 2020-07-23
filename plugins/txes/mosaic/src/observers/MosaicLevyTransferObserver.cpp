/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/catapult/cache_core/AccountStateCache.h"
#include "Observers.h"
#include "src/cache/LevyCache.h"
#include "src/model/MosaicLevy.h"
#include "src/utils/MosaicLevyCalculator.h"
#include "src/utils/MosaicLevyUtils.h"

namespace catapult { namespace observers {
		
	using Notification = model::BalanceTransferNotification<1>;
	
	void LevyBalanceTransferObserverDetail(const Notification& notification, const ObserverContext& context) {
		auto mosaicId = context.Resolvers.resolve(notification.MosaicId);
		auto pLevy = GetLevy(mosaicId, context);
		if(!pLevy)
			return;
		
		utils::MosaicLevyCalculatorFactory factory;
		Amount levyAmount = factory.getCalculator(pLevy->Type)(notification.Amount, pLevy->Fee);
		
		auto& cache = context.Cache.sub<cache::AccountStateCache>();
		auto senderIter = cache.find(notification.Sender);
		auto recipientIter = cache.find(pLevy->Recipient);
		
		auto& senderState = senderIter.get();
		auto& recipientState = recipientIter.get();
		
		if (NotifyMode::Commit == context.Mode) {
			senderState.Balances.debit(pLevy->MosaicId, levyAmount, context.Height);
			recipientState.Balances.credit(pLevy->MosaicId, levyAmount, context.Height);
		}else {
			recipientState.Balances.debit(pLevy->MosaicId, levyAmount, context.Height);
			senderState.Balances.credit(pLevy->MosaicId, levyAmount, context.Height);
		}
	}
	
	DEFINE_OBSERVER(LevyBalanceTransfer, Notification, [](const auto& notification, const ObserverContext& context) {
		LevyBalanceTransferObserverDetail(notification, context);
	});
}}