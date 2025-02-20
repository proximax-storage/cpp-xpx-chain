/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "PlaceSdaExchangeOfferMapper.h"
#include "RemoveSdaExchangeOfferMapper.h"
#include "mongo/src/MongoPluginManager.h"
#include "storages/MongoSdaExchangeCacheStorage.h"
#include "storages/MongoSdaOfferGroupCacheStorage.h"
#include "mongo/src/MongoReceiptPluginFactory.h"
#include "plugins/txes/exchange_sda/src/model/SdaExchangeReceiptType.h"

extern "C" PLUGIN_API
void RegisterMongoSubsystem(catapult::mongo::MongoPluginManager& manager) {
    // transaction support
    manager.addTransactionSupport(catapult::mongo::plugins::CreatePlaceSdaExchangeOfferTransactionMongoPlugin());
    manager.addTransactionSupport(catapult::mongo::plugins::CreateRemoveSdaExchangeOfferTransactionMongoPlugin());

    // cache storage support
    manager.addStorageSupport(catapult::mongo::plugins::CreateMongoSdaExchangeCacheStorage(
            manager.mongoContext(),
            manager.configHolder()));
    manager.addStorageSupport(catapult::mongo::plugins::CreateMongoSdaOfferGroupCacheStorage(
            manager.mongoContext(),
            manager.configHolder()));

    // receipt support
    manager.addReceiptSupport(catapult::mongo::CreateOfferCreationReceiptMongoPlugin(catapult::model::Receipt_Type_Sda_Offer_Created));
    manager.addReceiptSupport(catapult::mongo::CreateOfferExchangeReceiptMongoPlugin(catapult::model::Receipt_Type_Sda_Offer_Exchanged));
    manager.addReceiptSupport(catapult::mongo::CreateOfferRemovalReceiptMongoPlugin(catapult::model::Receipt_Type_Sda_Offer_Removed));
}