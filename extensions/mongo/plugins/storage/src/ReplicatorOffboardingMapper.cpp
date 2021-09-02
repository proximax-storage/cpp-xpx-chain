/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ReplicatorOffboardingMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/storage/src/model/ReplicatorOffboardingTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamReplicatorOffboardingTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "publicKey" << ToBinary(transaction.Signer);
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(ReplicatorOffboarding, StreamReplicatorOffboardingTransaction)
}}}
