/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MetadataMapper.h"
#include "storages/MongoMetadataCacheStorage.h"
#include "mongo/src/MongoPluginManager.h"

extern "C" PLUGIN_API
void RegisterMongoSubsystem(catapult::mongo::MongoPluginManager& manager) {
	// transaction support
	manager.addTransactionSupport(catapult::mongo::plugins::CreateAddressMetadataV1TransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateMosaicMetadataV1TransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateNamespaceMetadataV1TransactionMongoPlugin());

	// cache storage support
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoMetadataCacheStorage(
		manager.mongoContext(),
		manager.configHolder()));
}
