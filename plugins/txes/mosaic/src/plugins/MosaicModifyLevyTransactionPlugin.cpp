#include "src/model/MosaicNotifications.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "MosaicModifyLevyTransactionPlugin.h"
#include "src/model/MosaicModifyLevyTransaction.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
				case 1:
					sub.notify(MosaicModifyLevyNotification<1>(transaction.ModifyFlag,
							transaction.MosaicId, transaction.LevyInfo, transaction.Signer));
					break;

				default:
					CATAPULT_LOG(debug) << "invalid version of MosaicModifyLevyTransaciton: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(MosaicModifyLevy, Default, Publish)
}}
