/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "mongo/src/MongoTransactionPlugin.h"

namespace catapult { namespace mongo { namespace plugins {

	/// Creates a mongo remove harvester transaction plugin.
	PLUGIN_API
	std::unique_ptr<MongoTransactionPlugin> CreateRemoveHarvesterTransactionMongoPlugin();
}}}
