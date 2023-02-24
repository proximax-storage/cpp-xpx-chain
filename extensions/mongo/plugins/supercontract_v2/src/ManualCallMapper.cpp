/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ManualCallMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "src/model/ManualCallTransaction.h"
#include "CommonSupercontractStreams.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

    template<typename TTransaction>
    void StreamManualCallTransaction(bson_stream::document& builder, const TTransaction& transaction) {
        builder << "contractKey" << ToBinary(transaction.ContractKey);
		StreamManualCall(builder,
						 transaction.FileNamePtr(),
						 transaction.FileNameSize,
						 transaction.FunctionNamePtr(),
						 transaction.FunctionNameSize,
						 transaction.ActualArgumentsPtr(),
						 transaction.ActualArgumentsSize,
						 transaction.ServicePaymentsPtr(),
						 transaction.ServicePaymentsCount,
						 transaction.ExecutionCallPayment,
						 transaction.DownloadCallPayment);
    }

    DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(ManualCall, StreamManualCallTransaction)
}}}
