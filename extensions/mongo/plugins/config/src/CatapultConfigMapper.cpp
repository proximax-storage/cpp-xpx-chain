/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CatapultConfigMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/config/src/model/CatapultConfigTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamCatapultConfigTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder
				<< "applyHeightDelta" << ToInt64(transaction.ApplyHeightDelta)
				<< "blockChainConfig" << std::string((const char*)transaction.BlockChainConfigPtr(), transaction.BlockChainConfigSize);
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(CatapultConfig, StreamCatapultConfigTransaction)
}}}
