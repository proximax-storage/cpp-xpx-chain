/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StartExecuteMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "src/model/StartExecuteTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		void StreamMosaics(bson_stream::document& builder, const model::UnresolvedMosaic* pMosaic, size_t numMosaics) {
			auto mosaicsArray = builder << "mosaics" << bson_stream::open_array;
			for (auto i = 0u; i < numMosaics; ++i) {
				StreamMosaic(mosaicsArray, pMosaic->MosaicId, pMosaic->Amount);
				++pMosaic;
			}

			mosaicsArray << bson_stream::close_array;
		}
	}

	template<typename TTransaction>
	void StreamStartExecuteTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "superContract" << ToBinary(transaction.SuperContract);
		if (transaction.FunctionSize) {
			builder << "function" << ToBinary(transaction.FunctionPtr(), transaction.FunctionSize);
		}

		if (transaction.DataSize) {
			builder << "data" << ToBinary(transaction.DataPtr(), transaction.DataSize);
		}
		StreamMosaics(builder, transaction.MosaicsPtr(), transaction.MosaicsCount);
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(StartExecute, StreamStartExecuteTransaction)
}}}
