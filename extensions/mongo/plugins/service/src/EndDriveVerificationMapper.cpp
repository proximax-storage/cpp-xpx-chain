/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "EndDriveVerificationMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/service/src/model/EndDriveVerificationTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamEndDriveVerificationTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		auto array = builder << "verificationFailures" << bson_stream::open_array;
		auto failures = transaction.Transactions();
		for (auto iter = failures.begin(); iter != failures.end(); ++iter) {
			bson_stream::document failureBuilder;
			failureBuilder << "replicator" << ToBinary(iter->Replicator);

			auto blockHashArray = failureBuilder << "blockHashes" << bson_stream::open_array;
			auto pBlockHashes = iter->BlockHashesPtr();
			for (auto i = 0u; i < iter->BlockHashCount(); ++i)
				blockHashArray << ToBinary(pBlockHashes[i]);

			blockHashArray << bson_stream::close_array;
			array << failureBuilder;
		}

		array << bson_stream::close_array;
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(EndDriveVerification, StreamEndDriveVerificationTransaction)
}}}
