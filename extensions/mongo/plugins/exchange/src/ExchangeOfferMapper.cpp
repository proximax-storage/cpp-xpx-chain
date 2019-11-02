/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExchangeOfferMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/exchange/src/model/ExchangeOfferTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		void StreamOffer(bson_stream::array_context& context, const model::OfferWithDuration& offer) {
			context
				<< bson_stream::open_document
				<< "mosaicId" << ToInt64(offer.Mosaic.MosaicId)
				<< "mosaicAmount" << ToInt64(offer.Mosaic.Amount)
				<< "cost" << ToInt64(offer.Cost)
				<< "type" << utils::to_underlying_type(offer.Type)
				<< "duration" << ToInt64(offer.Duration)
				<< bson_stream::close_document;
		}

		void StreamOffers(bson_stream::document& builder, const model::OfferWithDuration* pOffer, size_t numOffers) {
			auto offerArray = builder << "offers" << bson_stream::open_array;
			for (auto i = 0u; i < numOffers; ++i, ++pOffer) {
				StreamOffer(offerArray, *pOffer);
			}
			offerArray << bson_stream::close_array;
		}

		template<typename TTransaction>
		void StreamExchangeOfferTransaction(bson_stream::document &builder, const TTransaction &transaction) {
			StreamOffers(builder, transaction.OffersPtr(), transaction.OfferCount);
		}
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(ExchangeOffer, StreamExchangeOfferTransaction)
}}}
