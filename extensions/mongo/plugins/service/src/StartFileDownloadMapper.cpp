/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StartFileDownloadMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/service/src/model/StartFileDownloadTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamStartFileDownloadTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "driveKey" << ToBinary(transaction.DriveKey);
		builder << "operationToken" << ToBinary(transaction.OperationToken);
		auto array = builder << "files" << bson_stream::open_array;
		auto pFile = transaction.FilesPtr();
		for (auto i = 0u; i < transaction.FileCount; ++i, ++pFile) {
			array
				<< bson_stream::open_document
				<< "fileHash" << ToBinary(pFile->FileHash)
				<< "fileSize" << static_cast<int64_t>(pFile->FileSize)
				<< bson_stream::close_document;
		}

		array << bson_stream::close_array;
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(StartFileDownload, StreamStartFileDownloadTransaction)
}}}
