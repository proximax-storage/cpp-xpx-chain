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
}