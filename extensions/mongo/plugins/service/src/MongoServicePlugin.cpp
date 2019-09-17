/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ServiceMapper.h"
#include "mongo/src/MongoPluginManager.h"
#include "storages/MongoDriveCacheStorage.h"
#include "storages/MongoFileCacheStorage.h"

extern "C" PLUGIN_API
void RegisterMongoSubsystem(catapult::mongo::MongoPluginManager& manager) {
	// transaction support
	manager.addTransactionSupport(catapult::mongo::plugins::CreateServiceTransactionMongoPlugin());

	// cache storage support
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoFileCacheStorage(
			manager.mongoContext(),
			manager.configHolder()));
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoDriveCacheStorage(
			manager.mongoContext(),
			manager.configHolder()));
}
