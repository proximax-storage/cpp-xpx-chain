/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "PrepareDriveMapper.h"
#include "DriveFileSystemMapper.h"
#include "JoinToDriveMapper.h"
#include "FilesDepositMapper.h"
#include "StartDriveVerificationMapper.h"
#include "EndDriveVerificationMapper.h"
#include "EndDriveMapper.h"
#include "DeleteRewardMapper.h"
#include "mongo/src/MongoPluginManager.h"
#include "mongo/src/MongoReceiptPluginFactory.h"
#include "storages/MongoDriveCacheStorage.h"
#include "plugins/txes/service/src/model/ServiceReceiptType.h"

extern "C" PLUGIN_API
void RegisterMongoSubsystem(catapult::mongo::MongoPluginManager& manager) {
	// transaction support
	manager.addTransactionSupport(catapult::mongo::plugins::CreatePrepareDriveTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateDriveFileSystemTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateJoinToDriveTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateFilesDepositTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateStartDriveVerificationTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateEndDriveVerificationTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateEndDriveTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateDeleteRewardTransactionMongoPlugin());

	// cache storage support
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoDriveCacheStorage(
		manager.mongoContext(),
		manager.configHolder()
	));

	manager.addReceiptSupport(catapult::mongo::CreateDriveStateReceiptMongoPlugin(catapult::model::Receipt_Type_Drive_State));
}
