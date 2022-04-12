/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CreateLiquidityProviderMapper.h";
#include "ManualRateChangeMapper.h"
#include "mongo/src/MongoPluginManager.h"
#include "storages/MongoLiquidityProviderCacheStorage.h"

extern "C" PLUGIN_API
void RegisterMongoSubsystem(catapult::mongo::MongoPluginManager& manager) {
	// transaction support
	manager.addTransactionSupport(catapult::mongo::plugins::CreateCreateLiquidityProviderTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateManualRateChangeTransactionMongoPlugin());

	// cache storage support
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoLiquidityProviderCacheStorage(
			manager.mongoContext(),
			manager.configHolder()));
}
