/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommonSupercontractStreams.h"

namespace catapult { namespace mongo { namespace plugins {

    void StreamServicePayments(bson_stream::document& builder, const model::UnresolvedMosaic* pServicePayments, uint16_t numServicePayments) {
        auto servicePaymentsArray = builder << "servicePayments" << bson_stream::open_array;
        for (auto i = 0u; i < numServicePayments; ++i) {
            StreamMosaic(servicePaymentsArray, pServicePayments->MosaicId, pServicePayments->Amount);
            ++pServicePayments;
        }
        servicePaymentsArray << bson_stream::close_array;
    }

	void StreamManualCall(
			bson_stream::document& builder,
			const uint8_t* pFileName,
			uint16_t fileNameSize,
			const uint8_t* pFunctionName,
			uint16_t functionNameSize,
			const uint8_t* pActualArguments,
			uint16_t actualArgumentsSize,
			const model::UnresolvedMosaic* pServicePayments,
			uint16_t numServicePayments,
			Amount executionCallPayment,
			Amount downloadCallPayment) {
		builder << "fileName" << ToBinary(pFileName, fileNameSize)
				<< "functionName" << ToBinary(pFunctionName, functionNameSize)
				<< "actualArguments" << ToBinary(pActualArguments, actualArgumentsSize);
		StreamServicePayments(builder, pServicePayments, numServicePayments);
		builder << "executionCallPayment" << ToInt64(executionCallPayment)
				<< "downloadCallPayment" << ToInt64(downloadCallPayment);
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

    void StreamProofOfExecution(bson_stream::document& builder, const model::RawProofOfExecution& pRawPoEx) {
        builder << "poEx" << bson_stream::open_document
                << "startBatchId" << static_cast<int64_t>(pRawPoEx.StartBatchId)
                << "T" << ToBinary(pRawPoEx.T.data(), pRawPoEx.T.size())
                << "R" << ToBinary(pRawPoEx.R.data(), pRawPoEx.R.size())
                << "F" << ToBinary(pRawPoEx.F.data(), pRawPoEx.F.size())
                << "K" << ToBinary(pRawPoEx.K.data(), pRawPoEx.K.size())
                << bson_stream::close_document;
    }

    void StreamOpinions(bson_stream::document& builder, size_t numCosigners, size_t numCalls, const model::RawProofOfExecution* pRawPoEx, const model::CallPayment* pCallPayments, const Key* pKeys, const Signature* pSignatures) {
        auto opinionsArray = builder << "opinions" << bson_stream::open_array;
        for (auto i = 0u; i < numCosigners; ++i) {

        	bson_stream::document opinionBuilder;

        	opinionBuilder << "publicKey" << ToBinary(*pKeys);
			pKeys++;

			opinionBuilder << "signature" << ToBinary(*pSignatures);
			pSignatures++;

            StreamProofOfExecution(opinionBuilder, *pRawPoEx);
            pRawPoEx++;

            StreamCallPayments(opinionBuilder, pCallPayments, numCalls);
			pCallPayments += numCalls;

			opinionsArray << opinionBuilder;
        }
        opinionsArray << bson_stream::close_array;
    }

}}}