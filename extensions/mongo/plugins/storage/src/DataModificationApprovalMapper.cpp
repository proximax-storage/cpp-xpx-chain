/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DataModificationApprovalMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/storage/src/model/DataModificationApprovalTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamDataModificationApprovalTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "driveKey" << ToBinary(transaction.DriveKey);
		builder << "dataModificationId" << ToBinary(transaction.DataModificationId);
		builder << "fileStructureCdi" << ToBinary(transaction.FileStructureCdi);
		builder << "modificationStatus" << static_cast<int8_t>(transaction.ModificationStatus);
		builder << "fileStructureSizeBytes" << static_cast<int64_t>(transaction.FileStructureSizeBytes);
		builder << "metaFilesSizeBytes" << static_cast<int64_t>(transaction.MetaFilesSizeBytes);
		builder << "usedDriveSizeBytes" << static_cast<int64_t>(transaction.UsedDriveSizeBytes);
		builder << "judgingKeysCount" << static_cast<int8_t>(transaction.JudgingKeysCount);
		builder << "overlappingKeysCount" << static_cast<int8_t>(transaction.OverlappingKeysCount);
		builder << "judgedKeysCount" << static_cast<int8_t>(transaction.JudgedKeysCount);

		// Streaming PublicKeys
		auto publicKeysArray = builder << "publicKeys" << bson_stream::open_array;
		auto pKey = transaction.PublicKeysPtr();
		const auto totalKeysCount = transaction.JudgingKeysCount + transaction.OverlappingKeysCount + transaction.JudgedKeysCount;
		for (auto i = 0; i < totalKeysCount; ++i, ++pKey)
			publicKeysArray << ToBinary(*pKey);
		publicKeysArray << bson_stream::close_array;

		// Streaming Signatures
		auto signaturesArray = builder << "signatures" << bson_stream::open_array;
		auto pSignature = transaction.SignaturesPtr();
		const auto totalJudgingKeysCount = transaction.JudgingKeysCount + transaction.OverlappingKeysCount;
		for (auto i = 0; i < totalJudgingKeysCount; ++i, ++pSignature)
			signaturesArray << ToBinary(*pSignature);
		signaturesArray << bson_stream::close_array;

		// Streaming PresentOpinions
		const auto totalJudgedKeysCount = transaction.OverlappingKeysCount + transaction.JudgedKeysCount;
		const auto presentOpinionByteCount = (totalJudgingKeysCount * totalJudgedKeysCount + 7) / 8;
		auto pPresentOpinions = transaction.PresentOpinionsPtr();
		auto presentOpinionsArray = builder << "presentOpinions" << bson_stream::open_array;
		for (auto i = 0; i < presentOpinionByteCount; ++i, ++pPresentOpinions)
			presentOpinionsArray << static_cast<uint8_t>(*pPresentOpinions);
		presentOpinionsArray << bson_stream::close_array;

		// Streaming Opinions
		auto opinionsArray = builder << "opinions" << bson_stream::open_array;
		auto pOpinion = transaction.OpinionsPtr();
		for (auto i = 0; i < transaction.OpinionElementCount; ++i, ++pOpinion)
			opinionsArray << static_cast<int64_t>(*pOpinion);
		opinionsArray << bson_stream::close_array;
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(DataModificationApproval, StreamDataModificationApprovalTransaction)
}}}
