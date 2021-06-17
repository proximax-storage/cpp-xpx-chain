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
		builder << "uploadOpinionPairCount" << static_cast<int16_t>(transaction.UploadOpinionPairCount);	// TODO: Remove?

		auto uploaderKeys = builder << "uploaderKeys" << bson_stream::open_array;
		auto pKey = transaction.UploaderKeysPtr();
		for (auto i = 0u; i < transaction.UploadOpinionPairCount; ++i, ++pKey)
			uploaderKeys << ToBinary(*pKey);
		uploaderKeys << bson_stream::close_array;

		auto uploadOpinion = builder << "uploadOpinion" << bson_stream::open_array;
		auto pPercent = transaction.UploadOpinionPtr();
		for (auto i = 0u; i < transaction.UploadOpinionPairCount; ++i, ++pPercent)
			uploadOpinion << *pPercent;
		uploadOpinion << bson_stream::close_array;
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(DataModificationSingleApproval, StreamDataModificationSingleApprovalTransaction)
}}}
