/**
*** FOR TRAINING PURPOSES ONLY
**/

#include "HelloTransactionPlugin.h"
#include "src/model/HelloNotification.h"
#include "src/model/HelloTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

        namespace {
            template<typename TTransaction>
            void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
                auto entityVersion = transaction.EntityVersion();
                switch (entityVersion) {
                    case 1: {

                        // Display message N times based from transaction parameters
                        for(auto i = 0; i < transaction.MessageCount; i++) {
                            CATAPULT_LOG(info) << "Hello Plugin " << i;
                        }

                        sub.notify(HelloMessageCountNotification<1>(transaction.MessageCount, transaction.SignerKey));
                        break;
                    }

                    default:
                        CATAPULT_LOG(debug) << "invalid version of HelloTransaction: " << transaction.EntityVersion();
                }
            }
        }


        // Expands to CreateHelloTransactionPlugin
        DEFINE_TRANSACTION_PLUGIN_FACTORY(Hello, Default, Publish)
    }}

