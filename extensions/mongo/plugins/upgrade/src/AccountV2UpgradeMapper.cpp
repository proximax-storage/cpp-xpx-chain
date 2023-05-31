/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "AccountV2UpgradeMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/upgrade/src/model/AccountV2UpgradeTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamAccountV2UpgradeTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder
				<< "newAccountPublicKey" << ToBinary(transaction.NewAccountPublicKey);
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(AccountV2Upgrade, StreamAccountV2UpgradeTransaction)
}}}
