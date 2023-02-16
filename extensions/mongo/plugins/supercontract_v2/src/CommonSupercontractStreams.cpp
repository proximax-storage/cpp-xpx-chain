/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommonSupercontractStreams.h"

namespace catapult { namespace mongo { namespace plugins {

    void StreamServicePayments(bson_stream::document& builder, const model::UnresolvedMosaic* pServicePayments, size_t numServicePayments) {
        auto servicePaymentsArray = builder << "servicePayments" << bson_stream::open_array;
        for (auto i = 0u; i < numServicePayments; ++i) {
            StreamMosaic(servicePaymentsArray, pServicePayments->MosaicId, pServicePayments->Amount);
            ++pServicePayments;
        }
        servicePaymentsArray << bson_stream::close_array;
    }

    void StreamCallDigests(bson_stream::document& builder, const model::ShortCallDigest* pShortCallDigest, size_t numShortCallDigests) {
        auto callDigestsArray = builder << "shortCallDigests" << bson_stream::open_array;
        for (auto i = 0u; i < numShortCallDigests; ++i) {
            callDigestsArray << bson_stream::open_document
                << "callId" << ToBinary(pShortCallDigest->CallId)
                << "manual" << pShortCallDigest->Manual
                << "block" << ToInt64(pShortCallDigest->Block)
                << bson_stream::close_document;
        }
        callDigestsArray << bson_stream::close_array;
    }

    void StreamCallDigests(bson_stream::document& builder, const model::ExtendedCallDigest* pExtendedCallDigest, size_t numExtendedCallDigests) {
        auto callDigestsArray = builder << "extendedCallDigests" << bson_stream::open_array;
        for (auto i = 0u; i < numExtendedCallDigests; ++i) {
            callDigestsArray << bson_stream::open_document
                << "callId" << ToBinary(pExtendedCallDigest->CallId)
                << "manual" << pExtendedCallDigest->Manual
                << "block" << ToInt64(pExtendedCallDigest->Block)
                << "status" << static_cast<int16_t>(pExtendedCallDigest->Status)
                << "releasedTransactionHash" << ToBinary(pExtendedCallDigest->ReleasedTransactionHash)
                << bson_stream::close_document;
        }
        callDigestsArray << bson_stream::close_array;
    }

    void StreamCallPayments(bson_stream::document& builder, const model::CallPayment* pCallPayments, size_t numCallPayments) {
        auto callPaymentsArray = builder << "callPayments" << bson_stream::open_array;
        for (auto i = 0u; i < numCallPayments; ++i) {
            callPaymentsArray << bson_stream::open_document
                << "executionPayment" << ToInt64(pCallPayments->ExecutionPayment)
                << "downloadPayment" << ToInt64(pCallPayments->DownloadPayment)
                << bson_stream::close_document;
        }
        callPaymentsArray << bson_stream::close_array;
    }

    void StreamProofOfExecution(bson_stream::document& builder, const model::RawProofOfExecution& rawPoEx) {
        builder << "poEx" << bson_stream::open_document
                << "startBatchId" << static_cast<int64_t>(rawPoEx.StartBatchId)
                << "T" << ToBinary(rawPoEx.T.data(), rawPoEx.T.size())
                << "R" << ToBinary(rawPoEx.R.data(), rawPoEx.R.size())
                << "F" << ToBinary(rawPoEx.F.data(), rawPoEx.F.size())
                << "K" << ToBinary(rawPoEx.K.data(), rawPoEx.K.size())
                << bson_stream::close_document;
    }


    void StreamProofOfExecution(bson_stream::document& builder, const model::RawProofOfExecution* rawPoEx) {
        builder << "poEx" << bson_stream::open_document
                << "startBatchId" << static_cast<int64_t>(rawPoEx->StartBatchId)
                << "T" << ToBinary(rawPoEx->T.data(), rawPoEx->T.size())
                << "R" << ToBinary(rawPoEx->R.data(), rawPoEx->R.size())
                << "F" << ToBinary(rawPoEx->F.data(), rawPoEx->F.size())
                << "K" << ToBinary(rawPoEx->K.data(), rawPoEx->K.size())
                << bson_stream::close_document;
    }

    void StreamOpinions(bson_stream::document& builder, const model::RawProofOfExecution* rawPoEx, const model::CallPayment* callPayments, size_t numCosigners, size_t numCalls) {
        auto opinionsArray = builder << "opinions" << bson_stream::open_array;
        for (auto i = 0u; i < numCosigners; ++i) {
            bson_stream::document poExBuilder;
            StreamProofOfExecution(poExBuilder, rawPoEx);
            opinionsArray << poExBuilder;
            bson_stream::document callPaymentsBuilder;
            StreamCallPayments(callPaymentsBuilder, callPayments, numCalls);
            opinionsArray << callPaymentsBuilder;
        }
        opinionsArray << bson_stream::close_array;
    }

}}}