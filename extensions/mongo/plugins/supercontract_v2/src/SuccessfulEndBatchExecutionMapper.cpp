/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SuccessfulEndBatchExecutionMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "src/model/SuccessfulEndBatchExecutionTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

    namespace {
        void StreamCallDigests(bson_stream::document& builder, const model::ExtendedCallDigest& pExtendedCallDigest, size_t numExtendedCallDigests) {
            auto callDigestsArray = builder << "callDigests" << bson_stream::open_array;
            for (auto i = 0u; i < numExtendedCallDigests; ++i) {
                callDigestsArray  << bson_stream::open_document
                    << "callId" << ToBinary(pExtendedCallDigest.CallId)
                    << "manual" << pExtendedCallDigest.Manual
                    << "status" << static_cast<int16_t>(pExtendedCallDigest.Status)
                    << "releasedTransactionHash" << ToBinary(pExtendedCallDigest.ReleasedTransactionHash)
                    << bson_stream::close_document;
            }
            callDigestsArray << bson_stream::close_array;
        }

        void StreamCallPayments(bson_stream::document& builder, const model::CallPayment& pCallPayments, size_t numCallPayments) {
            auto callPaymentsArray = builder << "callPayments" << bson_stream::open_array;
            for (auto i = 0u; i < numCallPayments; ++i) {
                callPaymentsArray << bson_stream::open_document
                    << "executionPayment" << ToInt64(pCallPayments.ExecutionPayment)
                    << "downloadPayment" << ToInt64(pCallPayments.DownloadPayment)
                    << bson_stream::close_document;
            }
            callPaymentsArray << bson_stream::close_array;
        }

        void StreamOpinions(bson_stream::document& builder, const model::RawProofOfExecution& rawPoEx, const model::CallPayment& callPayments, size_t numCosigners, size_t numCalls) {
            auto opinionsArray = builder << "opinions" << bson_stream::open_array;
            for (auto i = 0u; i < numCosigners; ++i) {
                opinionsArray << "poEx" << bson_stream::open_document
                    << "startBatchId" << static_cast<int64_t>(rawPoEx.StartBatchId)
                    << "T" << ToBinary(rawPoEx.T.data(), rawPoEx.T.size())
                    << "R" << ToBinary(rawPoEx.R.data(), rawPoEx.R.size())
                    << "F" << ToBinary(rawPoEx.F.data(), rawPoEx.F.size())
                    << "K" << ToBinary(rawPoEx.K.data(), rawPoEx.K.size())
                    << bson_stream::close_document;
                StreamCallPayments(builder, callPayments, numCalls);
            }
            opinionsArray << bson_stream::close_array;
        }
    }

    template<typename TTransaction>
    void StreamSuccessfulEndBatchExecutionTransaction(bson_stream::document& builder, const TTransaction& transaction) {
        builder << "contractKey" << ToBinary(transaction.ContractKey)
                << "batchId" << ToInt64(transaction.BatchId)
                << "automaticExecutionsNextBlockToCheck" << ToInt64(transaction.AutomaticExecutionsNextBlockToCheck)
                << "storageHash" << ToBinary(transaction.StorageHash)
                << "usedSizeBytes" << ToInt64(transaction.UsedSizeBytes)
                << "metaFilesSizeBytes" << ToInt64(transaction.MetaFilesSizeBytes)
                << "proofOfExecutionVerificationInformation" << ToBinary(transaction.ProofOfExecutionVerificationInformation.data(), transaction.ProofOfExecutionVerificationInformation.size());
        StreamCallDigests(builder, transaction.CallDigestsPtr(), transaction.CallsNumber);
        StreamOpinions(builder, transaction.ProofsOfExecutionPtr(), transaction.CallPaymentsPtr(), transaction.CosignersNumber, transaction.CallsNumber);

        auto cosignerArray = builder << "cosignersList" << bson_stream::open_array;
		auto pKey = transaction.PublicKeysPtr();
		for (auto i = 0; i < transaction.CosignersNumber; ++i, ++pKey)
			cosignerArray << ToBinary(*pKey);
		cosignerArray << bson_stream::close_array;
    }

    DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(SuccessfulEndBatchExecution, StreamSuccessfulEndBatchExecutionTransaction)
}}}
