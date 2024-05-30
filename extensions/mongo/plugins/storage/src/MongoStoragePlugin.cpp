/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "PrepareBcDriveMapper.h"
#include "DownloadMapper.h"
#include "DataModificationMapper.h"
#include "DataModificationApprovalMapper.h"
#include "DataModificationCancelMapper.h"
#include "ReplicatorOnboardingMapper.h"
#include "ReplicatorsCleanupMapper.h"
#include "ReplicatorTreeRebuildMapper.h"
#include "DriveClosureMapper.h"
#include "ReplicatorOffboardingMapper.h"
#include "FinishDownloadMapper.h"
#include "DownloadPaymentMapper.h"
#include "StoragePaymentMapper.h"
#include "DataModificationSingleApprovalMapper.h"
#include "VerificationPaymentMapper.h"
#include "DownloadApprovalMapper.h"
#include "EndDriveVerificationMapper.h"
#include "mongo/src/MongoPluginManager.h"
#include "mongo/src/MongoReceiptPluginFactory.h"
#include "src/model/StorageReceiptType.h"
#include "storages/MongoBcDriveCacheStorage.h"
#include "storages/MongoDownloadChannelCacheStorage.h"
#include "storages/MongoReplicatorCacheStorage.h"
#include "storages/MongoPriorityQueueCacheStorage.h"
#include "storages/MongoBootKeyReplicatorCacheStorage.h"

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
	manager.addTransactionSupport(catapult::mongo::plugins::CreateEndDriveVerificationTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateReplicatorsCleanupTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateReplicatorTreeRebuildTransactionMongoPlugin());

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
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoPriorityQueueCacheStorage(
			manager.mongoContext(),
			manager.configHolder()
	));
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoBootKeyReplicatorCacheStorage(
			manager.mongoContext(),
			manager.configHolder()
	));

	// receipt support
	manager.addReceiptSupport(catapult::mongo::CreateStorageReceiptMongoPlugin(catapult::model::Receipt_Type_Data_Modification_Approval_Download));
	manager.addReceiptSupport(catapult::mongo::CreateStorageReceiptMongoPlugin(catapult::model::Receipt_Type_Data_Modification_Approval_Refund));
	manager.addReceiptSupport(catapult::mongo::CreateStorageReceiptMongoPlugin(catapult::model::Receipt_Type_Data_Modification_Approval_Refund_Stream));
	manager.addReceiptSupport(catapult::mongo::CreateStorageReceiptMongoPlugin(catapult::model::Receipt_Type_Data_Modification_Approval_Upload));
	manager.addReceiptSupport(catapult::mongo::CreateStorageReceiptMongoPlugin(catapult::model::Receipt_Type_Data_Modification_Cancel_Pending_Owner));
	manager.addReceiptSupport(catapult::mongo::CreateStorageReceiptMongoPlugin(catapult::model::Receipt_Type_Data_Modification_Cancel_Pending_Replicator));
	manager.addReceiptSupport(catapult::mongo::CreateStorageReceiptMongoPlugin(catapult::model::Receipt_Type_Data_Modification_Cancel_Queued));
	manager.addReceiptSupport(catapult::mongo::CreateStorageReceiptMongoPlugin(catapult::model::Receipt_Type_Download_Approval));
	manager.addReceiptSupport(catapult::mongo::CreateStorageReceiptMongoPlugin(catapult::model::Receipt_Type_Download_Channel_Refund));
	manager.addReceiptSupport(catapult::mongo::CreateStorageReceiptMongoPlugin(catapult::model::Receipt_Type_Drive_Closure_Owner_Refund));
	manager.addReceiptSupport(catapult::mongo::CreateStorageReceiptMongoPlugin(catapult::model::Receipt_Type_Drive_Closure_Replicator_Modification));
	manager.addReceiptSupport(catapult::mongo::CreateStorageReceiptMongoPlugin(catapult::model::Receipt_Type_Drive_Closure_Replicator_Participation));
	manager.addReceiptSupport(catapult::mongo::CreateStorageReceiptMongoPlugin(catapult::model::Receipt_Type_End_Drive_Verification));
	manager.addReceiptSupport(catapult::mongo::CreateStorageReceiptMongoPlugin(catapult::model::Receipt_Type_Periodic_Payment_Owner_Refund));
	manager.addReceiptSupport(catapult::mongo::CreateStorageReceiptMongoPlugin(catapult::model::Receipt_Type_Periodic_Payment_Replicator_Modification));
	manager.addReceiptSupport(catapult::mongo::CreateStorageReceiptMongoPlugin(catapult::model::Receipt_Type_Periodic_Payment_Replicator_Participation));
}
