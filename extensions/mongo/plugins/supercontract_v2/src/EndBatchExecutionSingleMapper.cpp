/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "EndBatchExecutionSingleMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "src/model/EndBatchExecutionSingleTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

    namespace {
        void StreamProofOfExecution(bson_stream::document& builder, const model::RawProofOfExecution& rawPoEx) {
            builder << "poEx" << bson_stream::open_document
                    << "startBatchId" << static_cast<int64_t>(rawPoEx.StartBatchId)
                    << "T" << ToBinary(rawPoEx.T.data(), rawPoEx.T.size())
                    << "R" << ToBinary(rawPoEx.R.data(), rawPoEx.R.size())
                    << "F" << ToBinary(rawPoEx.F.data(), rawPoEx.F.size())
                    << "K" << ToBinary(rawPoEx.K.data(), rawPoEx.K.size()) 
                    << bson_stream::close_document;
        }
    }

    template<typename TTransaction>
    void StreamEndBatchExecutionSingleTransaction(bson_stream::document& builder, const TTransaction& transaction) {
        builder << "contractKey" << ToBinary(transaction.ContractKey)
                << "batchId" << ToInt64(transaction.BatchId)
                << "signer" << ToBinary(transaction.Signer);
        StreamProofOfExecution(builder, transaction.ProofOfExecution);
    }

    DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(EndBatchExecutionSingle, StreamEndBatchExecutionSingleTransaction)
}}}
