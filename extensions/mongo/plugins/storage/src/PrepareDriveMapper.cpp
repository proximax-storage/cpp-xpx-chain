/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "PrepareDriveMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/storage/src/model/PrepareDriveTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamPrepareDriveTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "owner" << ToBinary(transaction.Signer);
		builder << "driveSize" << static_cast<int64_t>(transaction.DriveSize);
		builder << "replicatorCount" << transaction.ReplicatorCount;
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(PrepareDrive, StreamPrepareDriveTransaction)
}}}
