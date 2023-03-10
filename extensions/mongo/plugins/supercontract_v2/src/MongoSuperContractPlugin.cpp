/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DeployContractMapper.h"
#include "ManualCallMapper.h"
#include "AutomaticExecutionsPaymentMapper.h"
#include "SuccessfulEndBatchExecutionMapper.h"
#include "EndBatchExecutionSingleMapper.h"
#include "SynchronizationSingleMapper.h"
#include "UnsuccessfulEndBatchExecutionMapper.h"
#include "mongo/src/MongoPluginManager.h"
#include "storages/MongoSuperContractCacheStorage.h"

extern "C" PLUGIN_API
void RegisterMongoSubsystem(catapult::mongo::MongoPluginManager& manager) {
	// transaction support
	manager.addTransactionSupport(catapult::mongo::plugins::CreateDeployContractTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateManualCallTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateAutomaticExecutionsPaymentTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateSuccessfulEndBatchExecutionTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateEndBatchExecutionSingleTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateSynchronizationSingleTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateUnsuccessfulEndBatchExecutionTransactionMongoPlugin());

	// cache storage support
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoSuperContract_v2CacheStorage(
		manager.mongoContext(),
		manager.configHolder()
	));
}
