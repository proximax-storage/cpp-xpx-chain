/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "UnsuccessfulEndBatchExecutionMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "CommonSupercontractStreams.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

    template<typename TTransaction>
    void StreamUnsuccessfulEndBatchExecutionTransaction(bson_stream::document& builder, const TTransaction& transaction) {
        builder << "contractKey" << ToBinary(transaction.ContractKey)
                << "batchId" << ToInt64(transaction.BatchId)
                << "automaticExecutionsNextBlockToCheck" << ToInt64(transaction.AutomaticExecutionsNextBlockToCheck);
        StreamCallDigests(builder, transaction.CallDigestsPtr(), transaction.CallsNumber);
        StreamOpinions(builder, transaction.ProofsOfExecutionPtr(), transaction.CallPaymentsPtr(), transaction.CosignersNumber, transaction.CallsNumber);
    }

    DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(UnsuccessfulEndBatchExecution, StreamUnsuccessfulEndBatchExecutionTransaction)

}}}
