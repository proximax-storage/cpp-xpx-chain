/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
#include "DeleteRewardTransactionPlugin.h"
#include "src/model/ServiceNotifications.h"
#include "src/model/DeleteRewardTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
				case 1: {
					sub.notify(DriveNotification<1>(transaction.Signer, transaction.Type));

                    std::vector<const model::DeletedFile*> files;
                    for (const auto& file : transaction.Transactions())
                        files.emplace_back(&file);

					sub.notify(DeleteRewardNotification<1>(files));
                    for (const auto& file : transaction.Transactions()) {
                        sub.notify(RewardNotification<1>(transaction.Signer, &file));
                    }

					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of DeleteRewardTransaction: "
										<< transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(DeleteReward, Default, Publish)
}}
