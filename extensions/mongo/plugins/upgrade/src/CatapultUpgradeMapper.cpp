/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CatapultUpgradeMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/upgrade/src/model/CatapultUpgradeTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamCatapultUpgradeTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder
				<< "upgradePeriod" << ToInt64(transaction.UpgradePeriod)
				<< "newCatapultVersion" << ToInt64(transaction.NewCatapultVersion);
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(CatapultUpgrade, StreamCatapultUpgradeTransaction)
}}}
