/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "FilesDepositMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/service/src/model/FilesDepositTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		void StreamFiles(bson_stream::document& builder, const std::vector<model::File>& files) {
			auto filesArray = builder << "files" << bson_stream::open_array;
			for (const auto& file : files) {
				filesArray << bson_stream::open_document
						 << "fileHash" << ToBinary(file.FileHash)
						 << bson_stream::close_document;
			}

			filesArray << bson_stream::close_array;
		}
	}

	template<typename TTransaction>
	void StreamFilesDepositTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "driveKey" << ToBinary(transaction.DriveKey);
		StreamFiles(builder, std::vector<model::File>(transaction.FilesPtr(), transaction.FilesPtr() + transaction.FilesCount));
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(FilesDeposit, StreamFilesDepositTransaction)
}}}
