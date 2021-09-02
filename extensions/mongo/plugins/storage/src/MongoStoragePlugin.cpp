/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "PrepareBcDriveMapper.h"
#include "DownloadMapper.h"
#include "DataModificationMapper.h"
#include "DataModificationApprovalMapper.h"
#include "DataModificationCancelMapper.h"
#include "ReplicatorOnboardingMapper.h"
#include "DriveClosureMapper.h"
#include "ReplicatorOffboardingMapper.h"
#include "FinishDownloadMapper.h"
#include "DownloadPaymentMapper.h"
#include "StoragePaymentMapper.h"
#include "DataModificationSingleApprovalMapper.h"
#include "VerificationPaymentMapper.h"
#include "DownloadApprovalMapper.h"
#include "FinishDriveVerificationMapper.h"
#include "mongo/src/MongoPluginManager.h"
#include "storages/MongoBcDriveCacheStorage.h"
#include "storages/MongoDownloadChannelCacheStorage.h"
#include "storages/MongoReplicatorCacheStorage.h"
#include "storages/MongoBlsKeysCacheStorage.h"

extern "C" PLUGIN_API
void RegisterMongoSubsystem(catapult::mongo::MongoPluginManager& manager) {
	// transaction support
	manager.addTransactionSupport(catapult::mongo::plugins::CreatePrepareBcDriveTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateDownloadTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateDataModificationTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateDataModificationApprovalTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateDataModificationCancelTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateReplicatorOnboardingTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateDriveClosureTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateReplicatorOffboardingTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateFinishDownloadTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateDownloadPaymentTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateStoragePaymentTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateDataModificationSingleApprovalTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateVerificationPaymentTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateDownloadApprovalTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateFinishDriveVerificationTransactionMongoPlugin());

	// cache storage support
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoBcDriveCacheStorage(
			manager.mongoContext(),
			manager.configHolder()
	));
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoDownloadChannelCacheStorage(
			manager.mongoContext(),
			manager.configHolder()
	));
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoReplicatorCacheStorage(
			manager.mongoContext(),
			manager.configHolder()
	));
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoBlsKeysCacheStorage(
			manager.mongoContext(),
			manager.configHolder()
	));
}
