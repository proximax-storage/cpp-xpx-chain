/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ReplicatorsCleanupMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/storage/src/model/ReplicatorsCleanupTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamReplicatorsCleanupTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "publicKey" << ToBinary(transaction.Signer);

		auto array = builder << "replicatorPublicKeys" << bson_stream::open_array;
		auto pKey = transaction.ReplicatorKeysPtr();
		for (auto i = 0; i < transaction.ReplicatorCount; ++i, ++pKey)
			array << ToBinary(*pKey);
		array << bson_stream::close_array;
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(ReplicatorsCleanup, StreamReplicatorsCleanupTransaction)
}}}
