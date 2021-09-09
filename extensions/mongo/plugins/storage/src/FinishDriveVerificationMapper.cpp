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

        // Streaming Provers Keys
        auto proversKeys = builder << "provers" << bson_stream::open_array;
        auto pKey = transaction.ProversPtr();
        for (auto i = 0u; i < transaction.VerifiersOpinionsCount; ++i, ++pKey)
            proversKeys << ToBinary(*pKey);
        proversKeys << bson_stream::close_array;

        // Streaming BlsSignatures
        auto blsSignaturesArray = builder << "blsSignatures" << bson_stream::open_array;
        auto pSignature = transaction.BlsSignaturesPtr();
        for (auto i = 0; i < transaction.VerifiersOpinionsCount; ++i, ++pSignature)
            blsSignaturesArray << ToBinary(*pSignature);
        blsSignaturesArray << bson_stream::close_array;

        // Streaming verification opinions
        auto verificationOpinion = builder << "verificationOpinion" << bson_stream::open_array;
        auto pPercent = transaction.VerifiersOpinionsPtr();
        for (auto i = 0u; i < transaction.VerifiersOpinionsCount; ++i, ++pPercent)
            verificationOpinion << *pPercent;
        verificationOpinion << bson_stream::close_array;
    }

    DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(FinishDriveVerification, StreamFinishDriveVerificationTransaction)
}}}