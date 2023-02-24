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
        builder << "contractKey" << ToBinary(transaction.ContractKey)
                << "fileName" << ToBinary(transaction.FileNamePtr(), transaction.FileNameSize)
                << "functionName" << ToBinary(transaction.FunctionNamePtr(), transaction.FunctionNameSize)
                << "actualArguments" << ToBinary(transaction.ActualArgumentsPtr(), transaction.ActualArgumentsSize);
        StreamServicePayments(builder, transaction.ServicePaymentsPtr(), transaction.ServicePaymentsCount);
        builder << "executionCallPayment" << ToInt64(transaction.ExecutionCallPayment)
                << "downloadCallPayment" << ToInt64(transaction.DownloadCallPayment);
    }

    DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(ManualCall, StreamManualCallTransaction)
}}}
