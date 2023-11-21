/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "AddHarvesterMapper.h"
#include "RemoveHarvesterMapper.h"
#include "mongo/src/MongoPluginManager.h"
#include "storages/MongoCommitteeCacheStorage.h"

extern "C" PLUGIN_API
void RegisterMongoSubsystem(catapult::mongo::MongoPluginManager& manager) {
	// transaction support
	manager.addTransactionSupport(catapult::mongo::plugins::CreateAddHarvesterTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateRemoveHarvesterTransactionMongoPlugin());

	// cache storage support
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoCommitteeCacheStorage(
			manager.mongoContext(),
			manager.configHolder()));
}
