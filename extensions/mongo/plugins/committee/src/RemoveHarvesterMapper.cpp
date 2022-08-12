/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "RemoveHarvesterMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/committee/src/model/RemoveHarvesterTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamRemoveHarvesterTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder
			<< "harvesterKey" << ToBinary(transaction.HarvesterKey);
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(RemoveHarvester, StreamRemoveHarvesterTransaction)
}}}
