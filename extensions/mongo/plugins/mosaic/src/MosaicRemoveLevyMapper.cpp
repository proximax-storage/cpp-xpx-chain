/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MosaicRemoveLevyMapper.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "plugins/txes/mosaic/src/model/MosaicRemoveLevyTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {
			
	namespace {
		
		template<typename TTransaction>
		void StreamTransaction(bson_stream::document& builder, const TTransaction& transaction) {
			builder << "mosaicId" << ToInt64(transaction.MosaicId);
		}
	}
			
	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(MosaicRemoveLevy, StreamTransaction)
}}}
