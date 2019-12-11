/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "EndDriveMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/service/src/model/ServiceTypes.h"
#include "plugins/txes/service/src/model/DeleteRewardTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

    void StreamUploadInfos(bson_stream::document& builder, const std::vector<model::ReplicatorUploadInfo>& infos) {
        auto infosStream = builder << "uploadInfos" << bson_stream::open_array;
        for (const auto& info : infos) {
            infosStream << bson_stream::open_document
                     << "participant" << ToBinary(info.Participant)
                     << "uploaded" << static_cast<int64_t>(info.Uploaded)
                     << bson_stream::close_document;
        }

        infosStream << bson_stream::close_array;
    }

	template<typename TTransaction>
	void StreamDeleteRewardTransaction(bson_stream::document& builder, const TTransaction& transaction) {
        auto deletedFiles = builder << "deletedFiles" << bson_stream::open_array;
        for (const auto& file : transaction.Transactions()) {
            bson_stream::document deletedFile;
            deletedFile << "fileHash" << ToBinary(file.FileHash)
                        << "size" << static_cast<int32_t>(file.Size);

            StreamUploadInfos(deletedFile, std::vector<model::ReplicatorUploadInfo>(file.InfosPtr(), file.InfosPtr() + file.InfosCount()));
            deletedFiles << deletedFile;
        }
        deletedFiles << bson_stream::close_array;
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(DeleteReward, StreamDeleteRewardTransaction)
}}}
