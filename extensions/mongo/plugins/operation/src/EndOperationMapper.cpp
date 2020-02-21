/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "EndOperationMapper.h"
#include "OperationMapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/operation/src/model/EndOperationTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamEndOperationTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		StreamBasicOperationTransaction(builder, transaction);
		builder << "operationToken" << ToBinary(transaction.OperationToken);
		builder << "result" << static_cast<int32_t>(transaction.Result);
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(EndOperation, StreamEndOperationTransaction)
}}}
