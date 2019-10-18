/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExchangeMapperUtils.h"

namespace catapult { namespace mongo { namespace plugins {

	bson_stream::array_context& StreamOffer(bson_stream::array_context& context, const model::Offer& offer) {
		context
			<< bson_stream::open_document
			<< "mosaicId" << ToInt64(offer.Mosaic.MosaicId)
			<< "mosaicAmount" << ToInt64(offer.Mosaic.Amount)
			<< "cost" << ToInt64(offer.Cost)
			<< bson_stream::close_document;
		return context;
	}

	void StreamOffers(bson_stream::document &builder, const model::Offer* pOffer, size_t numOffers) {
		auto offerArray = builder << "offers" << bson_stream::open_array;
		for (auto i = 0u; i < numOffers; ++i, ++pOffer) {
			StreamOffer(offerArray, *pOffer);
		}
		offerArray << bson_stream::close_array;
	}

	void StreamOffers(bson_stream::document &builder, const state::OfferMap& offers) {
		auto offerArray = builder << "offers" << bson_stream::open_array;
		for (const auto& pair : offers) {
			StreamOffer(offerArray, pair.second);
		}
		offerArray << bson_stream::close_array;
	}

	bson_stream::array_context& StreamMatchedOffer(bson_stream::array_context& context, const model::MatchedOffer& offer) {
		context
			<< bson_stream::open_document
			<< "mosaicId" << ToInt64(offer.Mosaic.MosaicId)
			<< "mosaicAmount" << ToInt64(offer.Mosaic.Amount)
			<< "cost" << ToInt64(offer.Cost)
			<< "transactionSigner" << ToBinary(offer.TransactionSigner)
			<< "transactionHash" << ToInt32(offer.TransactionHash)
			<< bson_stream::close_document;
		return context;
	}

	void StreamMatchedOffers(bson_stream::document &builder, const model::Offer* pOffer, size_t numOffers) {
		auto offerArray = builder << "matchedOffers" << bson_stream::open_array;
		for (auto i = 0u; i < numOffers; ++i, ++pOffer) {
			StreamOffer(offerArray, *pOffer);
		}
		offerArray << bson_stream::close_array;
	}
}}}
