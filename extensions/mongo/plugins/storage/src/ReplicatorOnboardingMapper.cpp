/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ReplicatorOnboardingMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/storage/src/model/ReplicatorOnboardingTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamReplicatorOnboardingTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "publicKey" << ToBinary(transaction.Signer);
		builder << "capacity" << ToInt64(transaction.Capacity);
		builder << "nodeBootKey" << ToBinary(transaction.NodeBootKey);
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(ReplicatorOnboarding, StreamReplicatorOnboardingTransaction)
}}}
