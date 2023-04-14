/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "AddDbrbProcessMapper.h"
#include "mongo/src/MongoPluginManager.h"
#include "storages/MongoDbrbViewCacheStorage.h"

extern "C" PLUGIN_API
void RegisterMongoSubsystem(catapult::mongo::MongoPluginManager& manager) {
	// transaction support
	manager.addTransactionSupport(catapult::mongo::plugins::CreateAddDbrbProcessTransactionMongoPlugin());

	// cache storage support
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoDbrbViewCacheStorage(
			manager.mongoContext(),
			manager.configHolder()
	));
}
