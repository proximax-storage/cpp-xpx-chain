/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "RemoveExchangeOfferMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/exchange/src/model/RemoveExchangeOfferTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		void StreamOfferMosaics(bson_stream::document &builder, const model::OfferMosaic* pMosaic, size_t numOffers) {
			auto offerArray = builder << "offerMosaics" << bson_stream::open_array;
			for (auto i = 0u; i < numOffers; ++i, ++pMosaic) {
				offerArray
					<< "mosaicId" << ToInt64(pMosaic->MosaicId)
					<< "offerType" << utils::to_underlying_type(pMosaic->OfferType);
			}
			offerArray << bson_stream::close_array;
		}

		template<typename TTransaction>
		void StreamRemoveExchangeOfferTransaction(bson_stream::document &builder, const TTransaction &transaction) {
			StreamOfferMosaics(builder, transaction.MosaicsPtr(), transaction.MosaicCount);
		}
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(RemoveExchangeOffer, StreamRemoveExchangeOfferTransaction)
}}}
