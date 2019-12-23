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
		auto addArray = builder << "verificationFailures" << bson_stream::open_array;
		const auto* pFailure = transaction.FailuresPtr();
		for (auto i = 0u; i < transaction.FailureCount; ++i, ++pFailure) {
			addArray << bson_stream::open_document
					 << "replicator" << ToBinary(pFailure->Replicator)
					 << "blockHash" << ToBinary(pFailure->BlockHash)
					 << bson_stream::close_document;
		}

		addArray << bson_stream::close_array;
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(EndDriveVerification, StreamEndDriveVerificationTransaction)
}}}
