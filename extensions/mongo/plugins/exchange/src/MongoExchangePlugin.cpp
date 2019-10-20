/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "BuyOfferMapper.h"
#include "RemoveOfferMapper.h"
#include "SellOfferMapper.h"
#include "mongo/src/MongoPluginManager.h"
#include "storages/MongoBuyOfferCacheStorage.h"
#include "storages/MongoDealCacheStorage.h"
#include "storages/MongoOfferDeadlineCacheStorage.h"
#include "storages/MongoSellOfferCacheStorage.h"

extern "C" PLUGIN_API
void RegisterMongoSubsystem(catapult::mongo::MongoPluginManager& manager) {
	// transaction support
	manager.addTransactionSupport(catapult::mongo::plugins::CreateBuyOfferTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateSellOfferTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateRemoveOfferTransactionMongoPlugin());

	// cache storage support
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoBuyOfferCacheStorage(
			manager.mongoContext(),
			manager.configHolder()));
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoSellOfferCacheStorage(
			manager.mongoContext(),
			manager.configHolder()));
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoDealCacheStorage(
			manager.mongoContext(),
			manager.configHolder()));
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoOfferDeadlineCacheStorage(
			manager.mongoContext(),
			manager.configHolder()));
}
