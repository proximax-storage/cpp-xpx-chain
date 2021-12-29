/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DataModificationSingleApprovalMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/storage/src/model/DataModificationSingleApprovalTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamDataModificationSingleApprovalTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "driveKey" << ToBinary(transaction.DriveKey);
		builder << "dataModificationId" << ToBinary(transaction.DataModificationId);
		builder << "publicKeysCount" << static_cast<int8_t>(transaction.PublicKeysCount);	// TODO: Remove?

		auto publicKeys = builder << "publicKeys" << bson_stream::open_array;
		auto pKey = transaction.PublicKeysPtr();
		for (auto i = 0u; i < transaction.PublicKeysCount; ++i, ++pKey)
			publicKeys << ToBinary(*pKey);
		publicKeys << bson_stream::close_array;

		auto opinions = builder << "opinions" << bson_stream::open_array;
		auto pOpinion = transaction.OpinionsPtr();
		for (auto i = 0u; i < transaction.PublicKeysCount; ++i, ++pOpinion)
			opinions << static_cast<int64_t>(*pOpinion);
		opinions << bson_stream::close_array;
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(DataModificationSingleApproval, StreamDataModificationSingleApprovalTransaction)
}}}
