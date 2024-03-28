/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "src/model/SuccessfulEndBatchExecutionTransaction.h"
#include "src/model/UnsuccessfulEndBatchExecutionTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

    void StreamServicePayments(bson_stream::document& builder, const model::UnresolvedMosaic* pServicePayments, uint16_t numServicePayments);

    void StreamManualCall(bson_stream::document& builder,
						  const uint8_t* pFileName,
						  uint16_t fileNameSize,
						  const uint8_t* pFunctionName,
						  uint16_t functionNameSize,
						  const uint8_t* pActualArguments,
						  uint16_t actualArgumentsSize,
						  const model::UnresolvedMosaic* pServicePayments,
						  uint16_t numServicePayments,
						  Amount executionCallPayment,
						  Amount downloadCallPayment);

    void StreamCallPayments(bson_stream::document& builder, const model::CallPayment* pCallPayments, size_t numCallPayments);

    void StreamProofOfExecution(bson_stream::document& builder, const model::RawProofOfExecution& rawPoEx);

    void StreamOpinions(bson_stream::document& builder, size_t numCosigners, size_t numCalls, const model::RawProofOfExecution* pRawPoEx, const model::CallPayment* pCallPayments, const Key* pKeys, const Signature* pSignatures);

}}}