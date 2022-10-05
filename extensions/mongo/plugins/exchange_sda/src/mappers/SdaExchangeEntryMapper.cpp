/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SdaExchangeEntryMapper.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

    // region ToDbModel

    namespace {
        void StreamSdaOfferBalance(bson_stream::document& builder, const state::MosaicsPair& pair, const state::SdaOfferBalance& offer) {
            builder
                << "mosaicIdGive" << ToInt64(pair.first)
                << "mosaicIdGet" << ToInt64(pair.second)
                << "currentMosaicGiveAmount" << ToInt64(offer.CurrentMosaicGive)
                << "currentMosaicGetAmount" << ToInt64(offer.CurrentMosaicGet)
                << "initialMosaicGiveAmount" << ToInt64(offer.InitialMosaicGive)
                << "initialMosaicGetAmount" << ToInt64(offer.InitialMosaicGet)
                << "deadline" << ToInt64(offer.Deadline);
        }

        void StreamSdaOfferBalances(bson_stream::document& builder, const state::SdaOfferBalanceMap& offers) {
            auto offerArray = builder << "sdaOfferBalances" << bson_stream::open_array;
            for (const auto& pair : offers) {
                bson_stream::document offerBuilder;
                StreamSdaOfferBalance(offerBuilder, pair.first, pair.second);
                offerArray << offerBuilder;
            }
            offerArray << bson_stream::close_array;
        }

        void StreamExpiredSdaOfferBalances(bson_stream::document& builder, const state::ExpiredSdaOfferBalanceMap& offers) {
            auto offerArray = builder << "expiredSdaOfferBalances" << bson_stream::open_array;
            for (const auto& pair : offers) {
                bson_stream::document expiredOfferBuilder;
                expiredOfferBuilder << "height" << ToInt64(pair.first);
                StreamSdaOfferBalances(expiredOfferBuilder, pair.second);
                offerArray << expiredOfferBuilder;
            }
            offerArray << bson_stream::close_array;
        }
    }

    bsoncxx::document::value ToDbModel(const state::SdaExchangeEntry& entry, const Address& ownerAddress) {
        bson_stream::document builder;
        auto doc = builder << "exchangesda" << bson_stream::open_document
                << "owner" << ToBinary(entry.owner())
                << "ownerAddress" << ToBinary(ownerAddress)
                << "version" << static_cast<int32_t>(entry.version());

        StreamSdaOfferBalances(builder, entry.sdaOfferBalances());
        StreamExpiredSdaOfferBalances(builder, entry.expiredSdaOfferBalances());

        return doc
                << bson_stream::close_document
                << bson_stream::finalize;
    }

    // endregion

    // region ToModel

    namespace {
        void ReadSdaOfferBalance(const bsoncxx::document::view& dbOffer, state::MosaicsPair& pair, state::SdaOfferBalance& offer) {
            pair.first = MosaicId{static_cast<uint64_t>(dbOffer["mosaicIdGive"].get_int64())};
            pair.second = MosaicId{static_cast<uint64_t>(dbOffer["mosaicIdGet"].get_int64())};
            offer.CurrentMosaicGive = Amount{static_cast<uint64_t>(dbOffer["currentMosaicGiveAmount"].get_int64())};
            offer.CurrentMosaicGet = Amount{static_cast<uint64_t>(dbOffer["currentMosaicGetAmount"].get_int64())};
            offer.InitialMosaicGive = Amount{static_cast<uint64_t>(dbOffer["initialMosaicGiveAmount"].get_int64())};
            offer.InitialMosaicGet = Amount{static_cast<uint64_t>(dbOffer["initialMosaicGetAmount"].get_int64())};
            offer.Deadline = Height{static_cast<uint64_t>(dbOffer["deadline"].get_int64())};
        }

        void ReadSdaOfferBalances(const bsoncxx::array::view& dbOffers, state::SdaOfferBalanceMap& offers) {
            for (const auto& dbOffer : dbOffers) {
                auto doc = dbOffer.get_document().view();
                state::MosaicsPair pair;
                state::SdaOfferBalance offer;
                ReadSdaOfferBalance(doc, pair, offer);
                offers.emplace(pair, offer);
            }
        }

        void ReadExpiredSdaOfferBalances(const bsoncxx::array::view& dbExpiredOffers, state::ExpiredSdaOfferBalanceMap& expiredOffers) {
            for (const auto& dbExpiredOffer : dbExpiredOffers) {
                auto doc = dbExpiredOffer.get_document().view();
                auto height = Height{static_cast<uint64_t>(doc["height"].get_int64())};
                state::SdaOfferBalanceMap offers;
                ReadSdaOfferBalances(doc["sdaOfferBalances"].get_array().value, offers);
                expiredOffers.emplace(height, offers);
            }
        }
    }

    state::SdaExchangeEntry ToSdaExchangeEntry(const bsoncxx::document::view& document) {
        auto dbSdaExchangeEntry = document["exchangesda"];
        Key owner;
        DbBinaryToModelArray(owner, dbSdaExchangeEntry["owner"].get_binary());
        auto version = ToUint32(dbSdaExchangeEntry["version"].get_int32());
        state::SdaExchangeEntry entry(owner, version);

        ReadSdaOfferBalances(dbSdaExchangeEntry["sdaOfferBalances"].get_array().value, entry.sdaOfferBalances());

        ReadExpiredSdaOfferBalances(dbSdaExchangeEntry["expiredSdaOfferBalances"].get_array().value, entry.expiredSdaOfferBalances());

        return entry;
    }

    // endregion
}}}