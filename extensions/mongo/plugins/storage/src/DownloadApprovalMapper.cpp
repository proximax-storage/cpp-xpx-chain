/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DownloadApprovalMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/storage/src/model/DownloadApprovalTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamDownloadApprovalTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "downloadChannelId" << ToBinary(transaction.DownloadChannelId);
		builder << "sequenceNumber" << static_cast<int16_t>(transaction.SequenceNumber);
		builder << "responseToFinishDownloadTransaction" << transaction.ResponseToFinishDownloadTransaction;

		// Streaming PublicKeys
		auto publicKeysArray = builder << "publicKeys" << bson_stream::open_array;
		auto pKey = transaction.PublicKeysPtr();
		for (auto i = 0; i < transaction.JudgedCount; ++i, ++pKey)
			publicKeysArray << ToBinary(*pKey);
		publicKeysArray << bson_stream::close_array;

		// Streaming OpinionIndices
		auto opinionIndicesArray = builder << "opinionIndices" << bson_stream::open_array;
		auto pIndex = transaction.OpinionIndicesPtr();
		for (auto i = 0; i < transaction.JudgingCount; ++i, ++pIndex)
			opinionIndicesArray << static_cast<int8_t>(*pIndex);
		opinionIndicesArray << bson_stream::close_array;

		// Streaming BlsSignatures
		auto blsSignaturesArray = builder << "blsSignatures" << bson_stream::open_array;
		auto pSignature = transaction.BlsSignaturesPtr();
		for (auto i = 0; i < transaction.OpinionCount; ++i, ++pSignature)
			blsSignaturesArray << ToBinary(*pSignature);
		blsSignaturesArray << bson_stream::close_array;

		// Streaming PresentOpinions
		auto presentOpinionsArray = builder << "presentOpinions" << bson_stream::open_array;
		auto pBlock = transaction.PresentOpinionsPtr();
		const auto presentOpinionByteCount = (transaction.OpinionCount * transaction.JudgedCount + 7) / 8;
		for (auto i = 0; i < presentOpinionByteCount; ++i, ++pBlock)
			presentOpinionsArray << static_cast<int8_t>(*pBlock);
		presentOpinionsArray << bson_stream::close_array;

		// Streaming Results
		auto opinionsArray = builder << "opinions" << bson_stream::open_array;
		auto pOpinion = transaction.OpinionsPtr();
		for (auto i = 0; i < transaction.OpinionElementCount; ++i, ++pOpinion)
			opinionsArray << static_cast<int64_t>(*pOpinion);
		opinionsArray << bson_stream::close_array;
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(DownloadApproval, StreamDownloadApprovalTransaction)
}}}
