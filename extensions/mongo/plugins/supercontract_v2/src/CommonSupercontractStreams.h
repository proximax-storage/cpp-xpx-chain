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

    void StreamServicePayments(bson_stream::document& builder, const model::UnresolvedMosaic* pServicePayments, size_t numServicePayments);

    void StreamCallDigests(bson_stream::document& builder, const model::ShortCallDigest* pShortCallDigest, size_t numShortCallDigests);

    void StreamCallDigests(bson_stream::document& builder, const model::ExtendedCallDigest* pExtendedCallDigest, size_t numExtendedCallDigests);

    void StreamCallPayments(bson_stream::document& builder, const model::CallPayment* pCallPayments, size_t numCallPayments);

    void StreamProofOfExecution(bson_stream::document& builder, const model::RawProofOfExecution& rawPoEx);

    void StreamProofOfExecution(bson_stream::document& builder, const model::RawProofOfExecution* rawPoEx);

    void StreamOpinions(bson_stream::document& builder, const model::RawProofOfExecution* rawPoEx, const model::CallPayment* callPayments, size_t numCosigners, size_t numCalls);

}}}