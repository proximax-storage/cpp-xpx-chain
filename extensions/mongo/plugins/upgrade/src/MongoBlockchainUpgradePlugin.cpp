/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "BlockchainUpgradeMapper.h"
#include "AccountV2UpgradeMapper.h"
#include "mongo/src/MongoPluginManager.h"
#include "storages/MongoBlockchainUpgradeCacheStorage.h"

extern "C" PLUGIN_API
void RegisterMongoSubsystem(catapult::mongo::MongoPluginManager& manager) {
	// transaction support
	manager.addTransactionSupport(catapult::mongo::plugins::CreateBlockchainUpgradeTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateAccountV2UpgradeTransactionMongoPlugin());

	// cache storage support
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoBlockchainUpgradeCacheStorage(
			manager.mongoContext(),
			manager.configHolder()));
}
