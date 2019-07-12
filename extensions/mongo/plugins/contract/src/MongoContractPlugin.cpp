/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ModifyContractMapper.h"
#include "mongo/src/MongoPluginManager.h"
#include "storages/MongoContractCacheStorage.h"
#include "storages/MongoReputationCacheStorage.h"

extern "C" PLUGIN_API
void RegisterMongoSubsystem(catapult::mongo::MongoPluginManager& manager) {
	// transaction support
	manager.addTransactionSupport(catapult::mongo::plugins::CreateModifyContractTransactionMongoPlugin());

	// cache storage support
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoReputationCacheStorage(
			manager.mongoContext()
			manager.configHolder()));
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoContractCacheStorage(
			manager.mongoContext(),
			manager.configHolder()));
}
