/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StreamStartMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/streaming/src/model/StreamStartTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamStreamStartTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "driveKey" << ToBinary(transaction.DriveKey);
		builder << "expectedUploadSize" << static_cast<int64_t>(transaction.ExpectedUploadSizeMegabytes);
		builder << "feedbackFeeAmount" << ToInt64(transaction.FeedbackFeeAmount);
		auto pFolderName = (const uint8_t*) (transaction.FolderNamePtr());
		builder << "folderName" << ToBinary(pFolderName, transaction.FolderNameSize);
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(StreamStart, StreamStreamStartTransaction)
}}}
