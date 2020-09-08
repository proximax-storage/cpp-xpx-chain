/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DeployMapper.h"
#include "StartExecuteMapper.h"
#include "EndExecuteMapper.h"
#include "UploadFileMapper.h"
#include "DeactivateMapper.h"
#include "mongo/src/MongoPluginManager.h"
#include "storages/MongoSuperContractCacheStorage.h"

extern "C" PLUGIN_API
void RegisterMongoSubsystem(catapult::mongo::MongoPluginManager& manager) {
	// transaction support
	manager.addTransactionSupport(catapult::mongo::plugins::CreateDeployTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateStartExecuteTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateEndExecuteTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateUploadFileTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateDeactivateTransactionMongoPlugin());

	// cache storage support
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoSuperContractCacheStorage(
		manager.mongoContext(),
		manager.configHolder()
	));
}
