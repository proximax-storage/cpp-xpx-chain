#include "Observers.h"
#include "src/cache/MosaicCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "src/model/MosaicLevy.h"
#include "src/utils/MosaicLevyCalculator.h"

namespace catapult { namespace observers {

		using Notification = model::BalanceTransferNotification<1>;

		void LevyTransferObserverDetail(
				const model::BalanceTransferNotification<1>& notification,
				const ObserverContext& context) {

			auto mosaicId = context.Resolvers.resolve(notification.MosaicId);

			auto& mosaicCache = context.Cache.sub<cache::MosaicCache>();
			auto mosaicIter = mosaicCache.find(mosaicId);
			if(!mosaicIter.tryGet()) return;
			
			auto& entry = mosaicIter.get();
			auto levy = entry.levy();

			utils::MosaicLevyCalculatorFactory factory;
			auto result = factory.getCalculator(levy.Type)(notification.Amount, levy.Fee);

			auto& cache = context.Cache.sub<cache::AccountStateCache>();
			auto senderIter = cache.find(notification.Sender);
			auto recipientIter = cache.find(context.Resolvers.resolve(levy.Recipient));

			auto& senderState = senderIter.get();
			if( !recipientIter.tryGet()) return;
			
			auto& recipientState = recipientIter.get();

			auto mosaicToUse = levy.MosaicId == model::UnsetMosaicId ? mosaicId : levy.MosaicId;
			if (NotifyMode::Commit == context.Mode) {
				senderState.Balances.debit(mosaicToUse, result.levyAmount, context.Height);
				recipientState.Balances.credit(mosaicToUse, result.levyAmount, context.Height);
			} else {
				recipientState.Balances.debit(mosaicToUse, result.levyAmount, context.Height);
				senderState.Balances.credit(mosaicToUse, result.levyAmount, context.Height);
			}
		}

		DEFINE_OBSERVER(LevyTransfer, model::BalanceTransferNotification<1>, [](const auto& notification, const ObserverContext& context) {
			LevyTransferObserverDetail(notification, context);
		});
}}