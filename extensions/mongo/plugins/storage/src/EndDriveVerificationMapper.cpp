/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "EndDriveVerificationMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/storage/src/model/EndDriveVerificationTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

    template<typename TElement>
    void StreamArray(bson_stream::document& builder, uint16_t count, const TElement* pElement, const std::string& name) {
		auto proversKeys = builder << name << bson_stream::open_array;
		for (auto i = 0u; i < count; ++i, ++pElement)
			proversKeys << ToBinary(*pElement);
		proversKeys << bson_stream::close_array;
    }

    template<typename TTransaction>
    void StreamEndDriveVerificationTransaction(bson_stream::document& builder, const TTransaction& transaction) {
        builder << "drive" << ToBinary(transaction.DriveKey);
        builder << "verificationTrigger" << ToBinary(transaction.VerificationTrigger);
        builder << "shardId" << static_cast<int64_t>(transaction.ShardId);

		StreamArray(builder, transaction.JudgingKeyCount, transaction.PublicKeysPtr(), "publicKeys");
		StreamArray(builder, transaction.JudgingKeyCount, transaction.SignaturesPtr(), "signatures");

		builder << "opinions" << ToBinary(transaction.OpinionsPtr(), (transaction.JudgingKeyCount * transaction.KeyCount + 7) / 8);
    }

    DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(EndDriveVerification, StreamEndDriveVerificationTransaction)
}}}