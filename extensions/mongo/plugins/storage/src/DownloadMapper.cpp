/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DownloadMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/storage/src/model/DownloadTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamDownloadTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "driveKey" << ToBinary(transaction.DriveKey);
		builder << "downloadSize" << static_cast<int64_t>(transaction.DownloadSize);
		builder << "feedbackFeeAmount" << ToInt64(transaction.FeedbackFeeAmount);

		auto array = builder << "listOfPublicKeys" << bson_stream::open_array;
		auto pKey = transaction.ListOfPublicKeysPtr();
		for (auto i = 0; i < transaction.ListOfPublicKeysSize; ++i, ++pKey)
			array << ToBinary(*pKey);
		array << bson_stream::close_array;
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(Download, StreamDownloadTransaction)
}}}
