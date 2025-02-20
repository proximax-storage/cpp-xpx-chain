/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "AddDbrbProcessMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/dbrb/src/model/AddDbrbProcessTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamAddDbrbProcessTransaction(bson_stream::document& builder, const TTransaction& transaction) {
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(AddDbrbProcess, StreamAddDbrbProcessTransaction)
}}}
