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

    template<typename TTransaction>
    void StreamEndDriveVerificationTransaction(bson_stream::document& builder, const TTransaction& transaction) {
        builder << "drive" << ToBinary(transaction.DriveKey);
        builder << "verificationTrigger" << ToBinary(transaction.VerificationTrigger);

        // Streaming Provers Keys
        auto proversKeys = builder << "provers" << bson_stream::open_array;
        auto pKey = transaction.ProversPtr();
        for (auto i = 0u; i < transaction.ProversCount; ++i, ++pKey)
            proversKeys << ToBinary(*pKey);
        proversKeys << bson_stream::close_array;

        // Streaming verification opinions
        auto verificationOpinions = builder << "verificationOpinions" << bson_stream::open_array;
        auto pOpinion = transaction.VerificationOpinionsPtr();
        for (auto i = 0u; i < transaction.VerificationOpinionsCount; ++i, ++pOpinion) {
            verificationOpinions << "verifier" << ToBinary((*pOpinion).Verifier);
            verificationOpinions << "blsSignature" << ToBinary((*pOpinion).BlsSignature);

            auto opinion = verificationOpinions << "verificationOpinion" << bson_stream::open_array;
            for (auto j = 0u; j < transaction.ProversCount-1; ++j) {
                opinion << "prover" << ToBinary((*pOpinion).Opinions[j].first);
                opinion << "opinion" << (*pOpinion).Opinions[j].second;
            }
            opinion << bson_stream::close_array;
        }
        verificationOpinions << bson_stream::close_array;
    }

    DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(EndDriveVerification, StreamEndDriveVerificationTransaction)
}}}