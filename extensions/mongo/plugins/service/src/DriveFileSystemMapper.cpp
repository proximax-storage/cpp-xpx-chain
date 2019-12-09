/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DriveFileSystemMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/service/src/model/DriveFileSystemTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
	    template<typename T>
		void StreamActions(bson_stream::document& builder, const std::string& actionsName, const std::vector<T>& actions) {
			auto addArray = builder << actionsName << bson_stream::open_array;
			for (const auto& action : actions) {
				addArray << bson_stream::open_document
						 << "fileHash" << ToBinary(action.FileHash)
						 << "fileSize" << static_cast<int64_t>(action.FileSize)
						 << bson_stream::close_document;
			}

			addArray << bson_stream::close_array;
		}
	}

	template<typename TTransaction>
	void StreamDriveFileSystemTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "driveKey" << ToBinary(transaction.DriveKey);
		builder << "rootHash" << ToBinary(transaction.RootHash);
		builder << "xorRootHash" << ToBinary(transaction.XorRootHash);
		StreamActions(builder, "addActions", std::vector<model::AddAction>(transaction.AddActionsPtr(), transaction.AddActionsPtr() + transaction.AddActionsCount));
		StreamActions(builder, "removeActions", std::vector<model::RemoveAction>(transaction.RemoveActionsPtr(), transaction.RemoveActionsPtr() + transaction.RemoveActionsCount));
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(DriveFileSystem, StreamDriveFileSystemTransaction)
}}}
