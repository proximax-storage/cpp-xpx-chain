/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NetworkConfigMapper.h"
#include "mongo/src/MongoPluginManager.h"
#include "storages/MongoNetworkConfigCacheStorage.h"

extern "C" PLUGIN_API
void RegisterMongoSubsystem(catapult::mongo::MongoPluginManager& manager) {
	// transaction support
	manager.addTransactionSupport(catapult::mongo::plugins::CreateNetworkConfigTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateNetworkConfigAbsoluteHeightTransactionMongoPlugin());

	// cache storage support
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoNetworkConfigCacheStorage(
			manager.mongoContext(),
			manager.configHolder()));
}
