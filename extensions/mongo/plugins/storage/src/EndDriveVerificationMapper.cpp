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
    void StreamVerificationOpinions(bson_stream::document& builder, const TTransaction& transaction) {
        auto verificationOpinions = builder << "verificationOpinions" << bson_stream::open_array;
        auto pOpinion = transaction.VerificationOpinionsPtr();
        for (auto i = 0u; i < transaction.VerificationOpinionsCount; ++i, ++pOpinion) {
            auto doc = verificationOpinions << bson_stream::open_document
                                            << "verifier" << (*pOpinion).Verifier
                                            << "signature" << ToBinary((*pOpinion).Signature);

            auto opinions = doc << "results" << bson_stream::open_array;
            for (auto j = 0u; j < transaction.ProversCount-1; ++j) {
                opinions
                        << bson_stream::open_document
                            << "prover" << (*pOpinion).Results[j].first
                            << "result" << (*pOpinion).Results[j].second
                        << bson_stream::close_document;
            }
            opinions << bson_stream::close_array;

            doc << bson_stream::close_document;
        }
        verificationOpinions << bson_stream::close_array;
    }

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
        StreamVerificationOpinions(builder, transaction);
    }

    DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(EndDriveVerification, StreamEndDriveVerificationTransaction)
}}}