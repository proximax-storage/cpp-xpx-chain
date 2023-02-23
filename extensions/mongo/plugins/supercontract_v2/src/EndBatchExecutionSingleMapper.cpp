/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "EndBatchExecutionSingleMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "src/model/EndBatchExecutionSingleTransaction.h"
#include "CommonSupercontractStreams.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

    template<typename TTransaction>
    void StreamEndBatchExecutionSingleTransaction(bson_stream::document& builder, const TTransaction& transaction) {
        builder << "contractKey" << ToBinary(transaction.ContractKey)
                << "batchId" << static_cast<int64_t>(transaction.BatchId);
        StreamProofOfExecution(builder, transaction.ProofOfExecution);
    }

    DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(EndBatchExecutionSingle, StreamEndBatchExecutionSingleTransaction)
}}}
