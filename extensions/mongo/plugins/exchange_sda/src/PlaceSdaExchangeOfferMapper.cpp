/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "PlaceSdaExchangeOfferMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/exchange_sda/src/model/PlaceSdaExchangeOfferTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

    namespace {
        void StreamSdaOffer(bson_stream::array_context& context, const model::SdaOfferWithDuration& offer) {
            context
                << bson_stream::open_document
                << "mosaicIdGive" << ToInt64(offer.MosaicGive.MosaicId)
                << "mosaicAmountGive" << ToInt64(offer.MosaicGive.Amount)
                << "mosaicIdGet" << ToInt64(offer.MosaicGet.MosaicId)
                << "mosaicAmountGet" << ToInt64(offer.MosaicGet.Amount)
                << "duration" << ToInt64(offer.Duration)
                << bson_stream::close_document;
        }

        void StreamSdaOffers(bson_stream::document& builder, const model::SdaOfferWithDuration* pOffer, size_t numOffers) {
            auto offerArray = builder << "offers" << bson_stream::open_array;
            for (auto i = 0u; i < numOffers; ++i, ++pOffer) {
                StreamSdaOffer(offerArray, *pOffer);
            }
            offerArray << bson_stream::close_array;
        }

        template<typename TTransaction>
        void StreamPlaceSdaExchangeOfferTransaction(bson_stream::document &builder, const TTransaction &transaction) {
            StreamSdaOffers(builder, transaction.SdaOffersPtr(), transaction.SdaOfferCount);
        }
    }

    DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(PlaceSdaExchangeOffer, StreamPlaceSdaExchangeOfferTransaction)
}}}