/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "OperationIdentifyMapper.h"
#include "StartOperationMapper.h"
#include "EndOperationMapper.h"
#include "storages/MongoOperationCacheStorage.h"
#include "mongo/src/MongoPluginManager.h"

extern "C" PLUGIN_API
void RegisterMongoSubsystem(catapult::mongo::MongoPluginManager& manager) {
	// transaction support
	manager.addTransactionSupport(catapult::mongo::plugins::CreateOperationIdentifyTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateStartOperationTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateEndOperationTransactionMongoPlugin());

	// cache storage support
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoOperationCacheStorage(
			manager.mongoContext(),
			manager.configHolder()));
}
