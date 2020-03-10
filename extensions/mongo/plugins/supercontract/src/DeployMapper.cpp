/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DeployMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "src/model/DeployTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamDeployTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "drive" << ToBinary(transaction.DriveKey);
		builder << "owner" << ToBinary(transaction.Owner);
		builder << "fileHash" << ToBinary(transaction.FileHash);
		builder << "vmVersion" << ToInt64(transaction.VmVersion);
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(Deploy, StreamDeployTransaction)
}}}
