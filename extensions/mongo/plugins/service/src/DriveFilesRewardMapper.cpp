/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/service/src/model/DriveFilesRewardTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamDriveFilesRewardTransaction(bson_stream::document& builder, const TTransaction& transaction) {
        auto infosStream = builder << "uploadInfos" << bson_stream::open_array;
        std::vector<model::UploadInfo> infos(transaction.UploadInfosPtr(), transaction.UploadInfosPtr() + transaction.UploadInfosCount);
        for (const auto& info : infos) {
            infosStream << bson_stream::open_document
                        << "participant" << ToBinary(info.Participant)
                        << "uploaded" << static_cast<int64_t>(info.Uploaded)
                        << bson_stream::close_document;
        }

        infosStream << bson_stream::close_array;
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(DriveFilesReward, StreamDriveFilesRewardTransaction)
}}}
