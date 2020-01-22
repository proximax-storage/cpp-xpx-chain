/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "EndFileDownloadMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/service/src/model/EndFileDownloadTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamEndFileDownloadTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "fileRecipient" << ToBinary(transaction.FileRecipient);
		builder << "operationToken" << ToBinary(transaction.OperationToken);
		auto array = builder << "files" << bson_stream::open_array;
		auto pFile = transaction.FilesPtr();
		for (auto i = 0u; i < transaction.FileCount; ++i, ++pFile)
			array << ToBinary(pFile->FileHash);

		array << bson_stream::close_array;
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(EndFileDownload, StreamEndFileDownloadTransaction)
}}}
