/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "PrepareDriveMapper.h"
#include "DownloadMapper.h"
#include "DataModificationMapper.h"
#include "DataModificationApprovalMapper.h"
#include "mongo/src/MongoPluginManager.h"
#include "mongo/src/MongoReceiptPluginFactory.h"
#include "storages/MongoDriveCacheStorage.h"
#include "storages/MongoDownloadCacheStorage.h"

extern "C" PLUGIN_API
void RegisterMongoSubsystem(catapult::mongo::MongoPluginManager& manager) {
	// transaction support
	manager.addTransactionSupport(catapult::mongo::plugins::CreatePrepareDriveTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateDownloadTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateDataModificationTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateDataModificationApprovalTransactionMongoPlugin());

	// cache storage support
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoDownloadCacheStorage(
		manager.mongoContext(),
		manager.configHolder()
	));
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoDriveCacheStorage(
			manager.mongoContext(),
			manager.configHolder()
	));
}
