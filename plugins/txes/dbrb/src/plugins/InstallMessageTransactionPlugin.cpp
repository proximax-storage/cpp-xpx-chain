/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "InstallMessageTransactionPlugin.h"
#include "src/model/DbrbNotifications.h"
#include "src/model/InstallMessageTransaction.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 1: {
				sub.notify(InstallMessageNotification<1>(
						transaction.MessageHash,
						transaction.ViewsCount,
						transaction.MostRecentViewSize,
						transaction.SignaturesCount,
						transaction.ViewSizesPtr(),
						transaction.ViewProcessIdsPtr(),
						transaction.MembershipChangesPtr(),
						transaction.SignaturesProcessIdsPtr(),
						transaction.SignaturesPtr()
				));
				break;
			}

			default:
				CATAPULT_LOG(debug) << "invalid version of InstallMessageTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(InstallMessage, Default, Publish)
}}
