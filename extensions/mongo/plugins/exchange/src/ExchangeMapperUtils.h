/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "mongo/src/mappers/MapperUtils.h"
#include "plugins/txes/exchange/src/state/ExchangeEntryUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	void StreamOffer(bson_stream::array_context& context, const model::Offer& pOffer);
	void StreamOffers(bson_stream::document &builder, const model::Offer* pOffer, size_t numOffers);
	void StreamOffers(bson_stream::document &builder, const state::OfferMap& offers);
	void StreamMatchedOffer(bson_stream::array_context& context, const model::MatchedOffer& offer);
	void StreamMatchedOffers(bson_stream::document &builder, const model::Offer* pOffer, size_t numOffers);
	void ReadOffers(const bsoncxx::array::view& dbOffers, state::OfferMap& offers);

	template<typename TTransaction>
	void StreamOfferTransaction(bson_stream::document &builder, const TTransaction &transaction) {
		builder << "deadline" << ToInt64(transaction.Deadline);
		StreamOffers(builder, transaction.OffersPtr(), transaction.OfferCount);
		StreamMatchedOffers(builder, transaction.MatchedOffersPtr(), transaction.OfferCount);
	}
}}}
