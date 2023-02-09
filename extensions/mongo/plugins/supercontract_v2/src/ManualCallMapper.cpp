/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ManualCallMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "src/model/ManualCallTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

    namespace {
        void StreamServicePayments(bson_stream::document& builder, const model::UnresolvedMosaic* pServicePayments, size_t numServicePayments) {
            auto servicePaymentsArray = builder << "servicePayments" << bson_stream::open_array;
            for (auto i = 0u; i < numServicePayments; ++i) {
                StreamMosaic(servicePaymentsArray, pServicePayments->MosaicId, pServicePayments->Amount);
                ++pServicePayments;
            }
            servicePaymentsArray << bson_stream::close_array;
        }
    }

    template<typename TTransaction>
    void StreamManualCallTransaction(bson_stream::document& builder, const TTransaction& transaction) {
        builder << "signer" << ToBinary(transaction.Signer) 
                << "contractKey" << ToBinary(transaction.ContractKey)
                << "automaticExecutionFileName" << ToBinary(transaction.AutomaticExecutionFileNamePtr(), transaction.AutomaticExecutionFileNameSize)
                << "automaticExecutionsFunctionName" << ToBinary(transaction.AutomaticExecutionFunctionNamePtr(), transaction.AutomaticExecutionFunctionNameSize)
                << "actualArguments" << ToBinary(transaction.ActualArgumentsPtr(), transaction.ActualArgumentsSize);
        StreamServicePayments(builder, transaction.ServicePaymentsPtr(), transaction.ServicePaymentsCount);
        builder << "executionCallPayment" << ToInt64(transaction.ExecutionCallPayment)
                << "downloadCallPayment" << ToInt64(transaction.DownloadCallPayment);
    }

    DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(ManualCall, StreamManualCallTransaction)
}}}
