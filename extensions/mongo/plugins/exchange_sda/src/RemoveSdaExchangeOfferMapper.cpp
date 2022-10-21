/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "RemoveSdaExchangeOfferMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/exchange_sda/src/model/RemoveSdaExchangeOfferTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

    namespace {
        void StreamSdaOffers(bson_stream::document &builder, const model::SdaOfferMosaic* pOffer, size_t numOffers) {
			auto offerArray = builder << "offers" << bson_stream::open_array;
			for (auto i = 0u; i < numOffers; ++i, ++pOffer) {
				offerArray
					<< bson_stream::open_document
					<< "mosaicIdGive" << ToInt64(pOffer->MosaicIdGive)
					<< "mosaicIdGet" << ToInt64(pOffer->MosaicIdGet)
					<< bson_stream::close_document;
			}
			offerArray << bson_stream::close_array;
		}

		template<typename TTransaction>
		void StreamRemoveSdaExchangeOfferTransaction(bson_stream::document &builder, const TTransaction &transaction) {
			StreamSdaOffers(builder, transaction.SdaOffersPtr(), transaction.SdaOfferCount);
		}
    }

    DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(RemoveSdaExchangeOffer, StreamRemoveSdaExchangeOfferTransaction)
}}}