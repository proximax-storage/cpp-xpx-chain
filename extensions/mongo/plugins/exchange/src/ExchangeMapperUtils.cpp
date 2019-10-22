/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExchangeMapperUtils.h"

namespace catapult { namespace mongo { namespace plugins {

	void StreamOffer(bson_stream::array_context& context, const model::Offer& offer) {
		context
			<< bson_stream::open_document
			<< "mosaicId" << ToInt64(offer.Mosaic.MosaicId)
			<< "mosaicAmount" << ToInt64(offer.Mosaic.Amount)
			<< "cost" << ToInt64(offer.Cost)
			<< bson_stream::close_document;
	}

	void StreamOffers(bson_stream::document &builder, const std::string& arrayName, const model::Offer* pOffer, size_t numOffers) {
		auto offerArray = builder << arrayName << bson_stream::open_array;
		for (auto i = 0u; i < numOffers; ++i, ++pOffer) {
			StreamOffer(offerArray, *pOffer);
		}
		offerArray << bson_stream::close_array;
	}

	void StreamOffers(bson_stream::document &builder, const std::string& arrayName, const state::OfferMap& offers) {
		auto offerArray = builder << arrayName << bson_stream::open_array;
		for (const auto& pair : offers) {
			StreamOffer(offerArray, pair.second);
		}
		offerArray << bson_stream::close_array;
	}

	void StreamMatchedOffer(bson_stream::array_context& context, const model::MatchedOffer& offer) {
		context
			<< bson_stream::open_document
			<< "mosaicId" << ToInt64(offer.Mosaic.MosaicId)
			<< "mosaicAmount" << ToInt64(offer.Mosaic.Amount)
			<< "cost" << ToInt64(offer.Cost)
			<< "transactionSigner" << ToBinary(offer.TransactionSigner)
			<< "transactionHash" << ToInt32(offer.TransactionHash)
			<< bson_stream::close_document;
	}

	void StreamMatchedOffers(bson_stream::document &builder, const model::MatchedOffer* pOffer, size_t numOffers) {
		auto offerArray = builder << "matchedOffers" << bson_stream::open_array;
		for (auto i = 0u; i < numOffers; ++i, ++pOffer) {
			StreamMatchedOffer(offerArray, *pOffer);
		}
		offerArray << bson_stream::close_array;
	}

	void ReadOffers(const bsoncxx::array::view& dbOffers, state::OfferMap& offers) {
		for (const auto& dbOffer : dbOffers) {
			auto doc = dbOffer.get_document().view();
			auto mosaicId = UnresolvedMosaicId{static_cast<uint64_t>(doc["mosaicId"].get_int64())};
			auto mosaicAmount = Amount{static_cast<uint64_t>(doc["mosaicAmount"].get_int64())};
			auto cost = Amount{static_cast<uint64_t>(doc["cost"].get_int64())};

			offers.emplace(mosaicId, model::Offer{model::UnresolvedMosaic{mosaicId, mosaicAmount}, cost});
		}
	}
}}}
