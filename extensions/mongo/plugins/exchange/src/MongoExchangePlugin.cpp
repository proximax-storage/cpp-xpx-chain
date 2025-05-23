/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExchangeOfferMapper.h"
#include "RemoveExchangeOfferMapper.h"
#include "ExchangeMapper.h"
#include "mongo/src/MongoPluginManager.h"
#include "storages/MongoExchangeCacheStorage.h"

extern "C" PLUGIN_API
void RegisterMongoSubsystem(catapult::mongo::MongoPluginManager& manager) {
	// transaction support
	manager.addTransactionSupport(catapult::mongo::plugins::CreateExchangeOfferTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateExchangeTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateRemoveExchangeOfferTransactionMongoPlugin());

	// cache storage support
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoExchangeCacheStorage(
			manager.mongoContext(),
			manager.configHolder()));
}
