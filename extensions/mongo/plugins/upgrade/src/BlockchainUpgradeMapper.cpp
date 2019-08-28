/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "BlockchainUpgradeMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/upgrade/src/model/BlockchainUpgradeTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamBlockchainUpgradeTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder
				<< "upgradePeriod" << ToInt64(transaction.UpgradePeriod)
				<< "newBlockchainVersion" << ToInt64(transaction.NewBlockchainVersion);
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(BlockchainUpgrade, StreamBlockchainUpgradeTransaction)
}}}
