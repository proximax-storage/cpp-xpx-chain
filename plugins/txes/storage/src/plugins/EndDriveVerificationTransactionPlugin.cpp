/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "sdk/src/extensions/ConversionExtensions.h"
#include "EndDriveVerificationTransactionPlugin.h"
#include "catapult/model/StorageNotifications.h"
#include "src/model/EndDriveVerificationTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

    namespace {
        template<typename TTransaction>
        auto CreatePublisher(const config::ImmutableConfiguration& config) {
            return [config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
                switch (transaction.EntityVersion()) {
                    case 1: {
                        sub.notify(EndDriveVerificationNotification<1>(
                                transaction.DriveKey,
                                transaction.VerificationTrigger,
                                transaction.ProversCount,
                                transaction.ProversPtr(),
                                transaction.VerificationOpinionsCount,
                                transaction.VerificationOpinionsPtr()
                        ));

                        break;
                    }

                    default:
                        CATAPULT_LOG(debug) << "invalid version of EndDriveVerificationTransaction: "
                                            << transaction.EntityVersion();
                }
            };
        }
    }

    DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(EndDriveVerification, Default, CreatePublisher, config::ImmutableConfiguration)
}}
