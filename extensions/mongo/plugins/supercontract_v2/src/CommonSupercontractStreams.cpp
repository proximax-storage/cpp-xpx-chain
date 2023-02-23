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
            ++pShortCallDigest;
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
            ++pExtendedCallDigest;
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
            ++pCallPayments;
        }
        callPaymentsArray << bson_stream::close_array;
    }

    void StreamProofOfExecution(bson_stream::document& builder, const model::RawProofOfExecution* pRawPoEx) {
        builder << "poEx" << bson_stream::open_document
                << "startBatchId" << static_cast<int64_t>(pRawPoEx->StartBatchId)
                << "T" << ToBinary(pRawPoEx->T.data(), pRawPoEx->T.size())
                << "R" << ToBinary(pRawPoEx->R.data(), pRawPoEx->R.size())
                << "F" << ToBinary(pRawPoEx->F.data(), pRawPoEx->F.size())
                << "K" << ToBinary(pRawPoEx->K.data(), pRawPoEx->K.size())
                << bson_stream::close_document;
    }

    void StreamOpinions(bson_stream::document& builder, const model::RawProofOfExecution* pRawPoEx, const model::CallPayment* pCallPayments, size_t numCosigners, size_t numCalls, const Key* pKeys, const Signature* pSignatures) {
        auto opinionsArray = builder << "opinions" << bson_stream::open_array;
        for (auto i = 0u; i < numCosigners; ++i) {
            bson_stream::document poExBuilder;
            StreamProofOfExecution(poExBuilder, pRawPoEx);
            opinionsArray << poExBuilder;
            bson_stream::document callPaymentsBuilder;
            StreamCallPayments(callPaymentsBuilder, pCallPayments, numCalls);
            opinionsArray << callPaymentsBuilder;

            auto publicKeysArray = builder << "publicKeys" << bson_stream::open_array;
            for (auto i = 0; i < numCosigners; ++i, ++pKeys)
                publicKeysArray << ToBinary(*pKeys);
            publicKeysArray << bson_stream::close_array;

            auto signaturesArray = builder << "signatures" << bson_stream::open_array;
            for (auto i = 0; i < numCosigners; ++i, ++pSignatures)
                signaturesArray << ToBinary(*pSignatures);
            signaturesArray << bson_stream::close_array;
        }
        opinionsArray << bson_stream::close_array;
    }

}}}