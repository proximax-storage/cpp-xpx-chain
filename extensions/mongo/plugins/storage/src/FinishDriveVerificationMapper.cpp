/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "FinishDriveVerificationMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/storage/src/model/FinishDriveVerificationTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

    template<typename TTransaction>
    void StreamFinishDriveVerificationTransaction(bson_stream::document& builder, const TTransaction& transaction) {
        builder << "drive" << ToBinary(transaction.DriveKey);
        builder << "verificationTrigger" << ToBinary(transaction.VerificationTrigger);

        auto uploaderKeys = builder << "provers" << bson_stream::open_array;
        auto pKey = transaction.ProversPtr();
        for (auto i = 0u; i < transaction.VerificationOpinionPairCount; ++i, ++pKey)
            uploaderKeys << ToBinary(*pKey);
        uploaderKeys << bson_stream::close_array;

        auto uploadOpinion = builder << "verificationOpinion" << bson_stream::open_array;
        auto pPercent = transaction.VerificationOpinionPtr();
        for (auto i = 0u; i < transaction.VerificationOpinionPairCount; ++i, ++pPercent)
            uploadOpinion << *pPercent;
        uploadOpinion << bson_stream::close_array;
    }

    DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(FinishDriveVerification, StreamFinishDriveVerificationTransaction)
}}}