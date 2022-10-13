/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "DeployTransactionPlugin.h"
#include "src/model/ContractNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

    namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
            switch (transaction.EntityVersion()) {
                case 1: {
                    sub.notify(DeployNotification<1>(
                        transaction.DriveKey,
                        transaction.FileNameSize,
                        transaction.FunctionNameSize,
                        transaction.ActualArgumentsSize,
                        transaction.ExecutionCallPayment,
                        transaction.DownloadCallPayment,
                        transaction.ServicePaymentCount,
                        transaction.ServicePaymentPtr(),
                        transaction.SingleApprovement,
                        transaction.AutomatedExecutionFileNameSize,
                        transaction.AutomatedExecutionFunctionNameSize,
                        transaction.AutomatedExecutionCallPayment,
                        transaction.AutomatedDownloadCallPayment,
                        transaction.AutomatedExecutionsNumber,
                        transaction.Assignee,
                    ));
                }

                default:
				CATAPULT_LOG(debug) << "invalid version of DeployTransaction: " << transaction.EntityVersion();
            }
        }
    }

    DEFINE_TRANSACTION_PLUGIN_FACTORY(Deploy, Default, Publish)
}}