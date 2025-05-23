/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SdaOfferGroupEntryMapper.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

    // region ToDbModel

    namespace {
        void StreamSdaOfferGroups(bson_stream::document& builder,  const state::SdaOfferGroupVector& offers) {
            auto array = builder << "sdaOfferGroup" << bson_stream::open_array;
            for (const auto& offer : offers) {
                array
                    << bson_stream::open_document
                    << "owner" << ToBinary(offer.Owner)
                    << "mosaicGiveAmount" << ToInt64(offer.MosaicGive)
                    << "deadline" << ToInt64(offer.Deadline)
                    << bson_stream::close_document;
            }

            array << bson_stream::close_array;
        }
    }

    bsoncxx::document::value ToDbModel(const state::SdaOfferGroupEntry& entry) {
        bson_stream::document builder;
        auto doc = builder << "sdaoffergroups" << bson_stream::open_document
                << "groupHash" << ToBinary(entry.groupHash());

        StreamSdaOfferGroups(builder, entry.sdaOfferGroup());

        return doc
                << bson_stream::close_document
                << bson_stream::finalize;
    }

    // endregion

    // region ToModel

    namespace {
        void ReadSdaOfferGroups(const bsoncxx::array::view& dbOfferVector, state::SdaOfferGroupVector& offers) {
            for (const auto& offerMap : dbOfferVector) {
                auto doc = offerMap.get_document().view();

                state::SdaOfferBasicInfo info;
                DbBinaryToModelArray(info.Owner, doc["owner"].get_binary());
                info.MosaicGive = Amount{static_cast<uint64_t>(doc["mosaicGiveAmount"].get_int64())};
                info.Deadline = Height{static_cast<uint64_t>(doc["deadline"].get_int64())};

                offers.emplace_back(info);
            }
        }
    }

    state::SdaOfferGroupEntry ToSdaOfferGroupEntry(const bsoncxx::document::view& document) {
        auto dbSdaOfferGroupEntry = document["sdaoffergroups"];
        Hash256 groupHash;
        DbBinaryToModelArray(groupHash, dbSdaOfferGroupEntry["groupHash"].get_binary());
        state::SdaOfferGroupEntry entry(groupHash);

        ReadSdaOfferGroups(dbSdaOfferGroupEntry["sdaOfferGroup"].get_array().value, entry.sdaOfferGroup());

        return entry;
    }

    // endregion
}}}