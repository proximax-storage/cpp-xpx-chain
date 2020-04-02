#include "Observers.h"
#include "src/cache/MosaicCache.h"
#include "src/model/MosaicLevy.h"
#include "src/model/MosaicModifyLevyTransaction.h"

namespace catapult { namespace observers {

	using Notification = model::MosaicModifyLevyNotification<1>;

	void ModifyLevyObserverDetail(
			const model::MosaicModifyLevyNotification<1>& notification,
			const ObserverContext& context) {

		auto& mosaicCache = context.Cache.sub<cache::MosaicCache>();
		auto mosaicIter = mosaicCache.find(notification.MosaicId);

		auto& entry = mosaicIter.get();
		auto& levy = entry.levyRef();

		if (NotifyMode::Commit == context.Mode) {
			if( notification.ModifyFlag & model::MosaicLevyModifyBitChangeType)
				levy.Type = notification.LevyInfo.Type;

			if( notification.ModifyFlag & model::MosaicLevyModifyBitChangeRecipient)
				levy.Recipient = notification.LevyInfo.Recipient;

			if( notification.ModifyFlag & model::MosaicLevyModifyBitChangeMosaicId)
				levy.MosaicId = notification.LevyInfo.MosaicId;

			if( notification.ModifyFlag & model::MosaicLevyModifyBitChangeLevyFee)
				levy.Fee = notification.LevyInfo.Fee;
		}
	}

	DEFINE_OBSERVER(ModifyLevy, Notification, [](const auto& notification, const ObserverContext& context) {
		ModifyLevyObserverDetail(notification, context);
	});
}}