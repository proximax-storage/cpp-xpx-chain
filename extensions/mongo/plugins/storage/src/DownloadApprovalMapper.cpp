/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DownloadApprovalMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/storage/src/model/DownloadApprovalTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamDownloadApprovalTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "downloadChannelId" << ToBinary(transaction.DownloadChannelId);
		builder << "responseToFinishDownloadTransaction" << transaction.ResponseToFinishDownloadTransaction;
		builder << "replicatorUploadOpinion" << static_cast<int64_t>(transaction.ReplicatorUploadOpinion);
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(DownloadApproval, StreamDownloadApprovalTransaction)
}}}
