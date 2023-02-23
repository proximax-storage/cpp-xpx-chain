/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SuccessfulEndBatchExecutionMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "CommonSupercontractStreams.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

    template<typename TTransaction>
    void StreamSuccessfulEndBatchExecutionTransaction(bson_stream::document& builder, const TTransaction& transaction) {
        builder << "contractKey" << ToBinary(transaction.ContractKey)
                << "batchId" << static_cast<int64_t>(transaction.BatchId)
                << "automaticExecutionsNextBlockToCheck" << ToInt64(transaction.AutomaticExecutionsNextBlockToCheck)
                << "storageHash" << ToBinary(transaction.StorageHash)
                << "usedSizeBytes" << static_cast<int64_t>(transaction.UsedSizeBytes)
                << "metaFilesSizeBytes" << static_cast<int64_t>(transaction.MetaFilesSizeBytes)
                << "proofOfExecutionVerificationInformation" << ToBinary(transaction.ProofOfExecutionVerificationInformation.data(), transaction.ProofOfExecutionVerificationInformation.size());
        StreamCallDigests(builder, transaction.CallDigestsPtr(), transaction.CallsNumber);
        StreamOpinions(builder, transaction.ProofsOfExecutionPtr(), transaction.CallPaymentsPtr(), transaction.CosignersNumber, transaction.CallsNumber, transaction.PublicKeysPtr(), transaction.SignaturesPtr());

        auto cosignerArray = builder << "cosignersList" << bson_stream::open_array;
        auto pKey = transaction.PublicKeysPtr();
        for (auto i = 0; i < transaction.CosignersNumber; ++i, ++pKey)
            cosignerArray << ToBinary(*pKey);
        cosignerArray << bson_stream::close_array;
    }

    DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(SuccessfulEndBatchExecution, StreamSuccessfulEndBatchExecutionTransaction)

}}}
