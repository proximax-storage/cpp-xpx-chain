/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "mongo/tests/test/MongoPluginTestUtils.h"
#include "plugins/txes/exchange_sda/src/model/SdaExchangeEntityType.h"
#include "plugins/txes/exchange_sda/src/model/SdaExchangeReceiptType.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

    namespace {
        struct MongoSdaExchangePluginTraits {
            public:
            static constexpr auto RegisterSubsystem = RegisterMongoSubsystem;

            static std::vector<model::EntityType> GetTransactionTypes() {
                return {
                    model::Entity_Type_Place_Sda_Exchange_Offer,
                    model::Entity_Type_Remove_Sda_Exchange_Offer,
                };
            }

            static std::vector<model::ReceiptType> GetReceiptTypes() {
                return {
                    model::Receipt_Type_Sda_Offer_Created,
                    model::Receipt_Type_Sda_Offer_Exchanged,
                    model::Receipt_Type_Sda_Offer_Removed
                };
            }

            static std::string GetStorageName() {
                return "{ ExchangeSdaCache, SdaOfferGroupCache }";
            }
        };
    }

    DEFINE_MONGO_PLUGIN_TESTS(MongoSdaExchangePluginTests, MongoSdaExchangePluginTraits)
}}}