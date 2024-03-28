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

	namespace {
		void StreamCallDigests(
				bson_stream::document& builder,
				const model::ExtendedCallDigest* pExtendedCallDigest,
				size_t numExtendedCallDigests) {
			auto callDigestsArray = builder << "callDigests" << bson_stream::open_array;
			for (auto i = 0u; i < numExtendedCallDigests; ++i) {
				callDigestsArray << bson_stream::open_document << "callId" << ToBinary(pExtendedCallDigest->CallId)
								 << "manual" << pExtendedCallDigest->Manual << "block"
								 << ToInt64(pExtendedCallDigest->Block) << "status"
								 << static_cast<int16_t>(pExtendedCallDigest->Status) << "releasedTransactionHash"
								 << ToBinary(pExtendedCallDigest->ReleasedTransactionHash)
								 << bson_stream::close_document;
				++pExtendedCallDigest;
			}
			callDigestsArray << bson_stream::close_array;
		}
	} // namespace

	template<typename TTransaction>
	void StreamSuccessfulEndBatchExecutionTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "contractKey" << ToBinary(transaction.ContractKey)
				<< "batchId" << static_cast<int64_t>(transaction.BatchId)
				<< "automaticExecutionsNextBlockToCheck" << ToInt64(transaction.AutomaticExecutionsNextBlockToCheck)
				<< "storageHash" << ToBinary(transaction.StorageHash)
				<< "usedSizeBytes" << static_cast<int64_t>(transaction.UsedSizeBytes)
				<< "metaFilesSizeBytes"	<< static_cast<int64_t>(transaction.MetaFilesSizeBytes)
				<< "proofOfExecutionVerificationInformation"
									<< ToBinary(transaction.ProofOfExecutionVerificationInformation.data(),
												transaction.ProofOfExecutionVerificationInformation.size());
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

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(SuccessfulEndBatchExecution, StreamSuccessfulEndBatchExecutionTransaction)

}}} // namespace catapult::mongo::plugins
