/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StartOperationMapper.h"
#include "OperationMapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/operation/src/model/StartOperationTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		void StreamExecutors(bson_stream::document& builder, const Key* pExecutor, uint8_t count) {
			auto array = builder << "executors" << bson_stream::open_array;
			for (auto i = 0u; i < count; ++i, ++pExecutor)
				array << ToBinary(*pExecutor);

			array << bson_stream::close_array;
		}
	}

	template<typename TTransaction>
	void StreamStartOperationTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		StreamBasicStartOperationTransaction(builder, transaction);
		StreamExecutors(builder, transaction.ExecutorsPtr(), transaction.ExecutorCount);
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(StartOperation, StreamStartOperationTransaction)
}}}
