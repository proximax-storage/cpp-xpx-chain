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

	void StreamCallDigests(bson_stream::document& builder, const model::ShortCallDigest* pShortCallDigest, size_t numShortCallDigests) {
		auto callDigestsArray = builder << "callDigests" << bson_stream::open_array;
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

	template<typename TTransaction>
	void StreamUnsuccessfulEndBatchExecutionTransaction(
			bson_stream::document& builder,
			const TTransaction& transaction) {
		builder << "contractKey" << ToBinary(transaction.ContractKey)
				<< "batchId" << static_cast<int64_t>(transaction.BatchId)
				<< "automaticExecutionsNextBlockToCheck" << ToInt64(transaction.AutomaticExecutionsNextBlockToCheck);
		StreamCallDigests(builder, transaction.CallDigestsPtr(), transaction.CallsNumber);
		StreamOpinions(
				builder,
				transaction.CosignersNumber,
				transaction.CallsNumber,
				transaction.ProofsOfExecutionPtr(),
				transaction.CallPaymentsPtr(),
				transaction.PublicKeysPtr(),
				transaction.SignaturesPtr());
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(
			UnsuccessfulEndBatchExecution,
			StreamUnsuccessfulEndBatchExecutionTransaction)

}}} // namespace catapult::mongo::plugins
