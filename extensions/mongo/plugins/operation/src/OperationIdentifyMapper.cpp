/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "OperationIdentifyMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/operation/src/model/OperationIdentifyTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamOperationIdentifyTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "operationToken" << ToBinary(transaction.OperationToken);
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(OperationIdentify, StreamOperationIdentifyTransaction)
}}}
