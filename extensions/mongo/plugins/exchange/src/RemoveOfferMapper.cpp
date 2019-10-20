/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "RemoveOfferMapper.h"
#include "ExchangeMapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/exchange/src/model/RemoveOfferTransaction.h"

namespace catapult { namespace mongo { namespace plugins {

	namespace {
#define DEFINE_STREAM_OFFERS_FUNC(FUNC_NAME, ARRAY_NAME) \
		void FUNC_NAME(bson_stream::document &builder, const utils::ShortHash* pHash, size_t numOffers) { \
			auto offerArray = builder << #ARRAY_NAME << bson_stream::open_array; \
			for (auto i = 0u; i < numOffers; ++i, ++pHash) { \
				offerArray << ToInt32(*pHash); \
			} \
			offerArray << bson_stream::close_array; \
		}

		DEFINE_STREAM_OFFERS_FUNC(StreamBuyOfferHashes, buyOffers)
		DEFINE_STREAM_OFFERS_FUNC(StreamSellOfferHashes, sellOffers)

		template<typename TTransaction>
		void StreamRemoveOfferTransaction(bson_stream::document &builder, const TTransaction &transaction) {
			StreamBuyOfferHashes(builder, transaction.BuyOfferHashesPtr(), transaction.BuyOfferCount);
			StreamSellOfferHashes(builder, transaction.SellOfferHashesPtr(), transaction.SellOfferCount);
		}
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(RemoveOffer, StreamRemoveOfferTransaction)
}}}
