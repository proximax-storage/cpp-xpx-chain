/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "LockFundMapper.h"
#include "storages/MongoLockFundCacheStorage.h"
#include "LockFundReceiptMapper.h"
#include "mongo/src/MongoPluginManager.h"


extern "C" PLUGIN_API
void RegisterMongoSubsystem(catapult::mongo::MongoPluginManager& manager) {
	// transaction support
	manager.addTransactionSupport(catapult::mongo::plugins::CreateLockFundTransferTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateLockFundCancelUnlockTransactionMongoPlugin());

	// cache storage support
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoLockFundCacheStorage(
			manager.mongoContext(),
			manager.configHolder()));

	manager.addReceiptSupport(catapult::mongo::plugins::CreateLockFundReceiptMongoPlugin());
}
