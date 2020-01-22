/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "PrepareDriveMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/service/src/model/PrepareDriveTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamPrepareDriveTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "drive" << ToBinary(transaction.DriveKey);
		builder << "duration" << ToInt64(transaction.Duration);
		builder << "billingPeriod" << ToInt64(transaction.BillingPeriod);
		builder << "billingPrice" << ToInt64(transaction.BillingPrice);
		builder << "driveSize" << static_cast<int64_t>(transaction.DriveSize);
		builder << "replicas" << transaction.Replicas;
		builder << "minReplicators" << transaction.MinReplicators;
		builder << "percentApprovers" << transaction.PercentApprovers;
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(PrepareDrive, StreamPrepareDriveTransaction)
}}}
