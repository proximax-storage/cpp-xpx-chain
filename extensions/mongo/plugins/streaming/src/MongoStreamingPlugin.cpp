/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StreamStartMapper.h"
#include "StreamFinishMapper.h"
#include "StreamPaymentMapper.h"
#include "UpdateDriveSizeMapper.h"
#include "mongo/src/MongoPluginManager.h"

extern "C" PLUGIN_API
		void RegisterMongoSubsystem(catapult::mongo::MongoPluginManager& manager) {
	// transaction support
	manager.addTransactionSupport(catapult::mongo::plugins::CreateStreamStartTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateStreamFinishTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateStreamPaymentTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateUpdateDriveSizeTransactionMongoPlugin());
}
