/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExchangeEntryMapper.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	namespace {
		void StreamOffer(bson_stream::array_context& context, const MosaicId& mosaicId, const state::OfferBase& offer) {
			context
				<< "mosaicId" << ToInt64(mosaicId)
				<< "amount" << ToInt64(offer.Amount)
				<< "initialAmount" << ToInt64(offer.InitialAmount)
				<< "initialCost" << ToInt64(offer.InitialCost)
				<< "deadline" << ToInt64(offer.Deadline)
				<< "expiryHeight" << ToInt64(offer.ExpiryHeight);
		}

		void StreamBuyOffers(bson_stream::document& builder, const state::BuyOfferMap& offers) {
			auto offerArray = builder << "buyOffers" << bson_stream::open_array;
			for (const auto& pair : offers) {
				offerArray << bson_stream::open_document;
				StreamOffer(offerArray, pair.first, pair.second);
				offerArray
					<< "residualCost" << ToInt64(pair.second.ResidualCost)
					<< bson_stream::close_document;
			}
			offerArray << bson_stream::close_array;
		}

		void StreamSellOffers(bson_stream::document& builder, const state::SellOfferMap& offers) {
			auto offerArray = builder << "sellOffers" << bson_stream::open_array;
			for (const auto& pair : offers) {
				offerArray << bson_stream::open_document;
				StreamOffer(offerArray, pair.first, pair.second);
				offerArray << bson_stream::close_document;
			}
			offerArray << bson_stream::close_array;
		}
	}

	bsoncxx::document::value ToDbModel(const state::ExchangeEntry& entry) {
		bson_stream::document builder;
		auto doc = builder << "exchange" << bson_stream::open_document
				<< "owner" << ToBinary(entry.owner());
		StreamBuyOffers(builder, entry.buyOffers());
		StreamSellOffers(builder, entry.sellOffers());

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	namespace {
		void ReadOffer(const bsoncxx::document::view& dbOffer, MosaicId& mosaicId, state::OfferBase& offer) {
			mosaicId = MosaicId{static_cast<uint64_t>(dbOffer["mosaicId"].get_int64())};
			offer.Amount = Amount{static_cast<uint64_t>(dbOffer["amount"].get_int64())};
			offer.InitialAmount = Amount{static_cast<uint64_t>(dbOffer["initialAmount"].get_int64())};
			offer.InitialCost = Amount{static_cast<uint64_t>(dbOffer["initialCost"].get_int64())};
			offer.Deadline = Height{static_cast<uint64_t>(dbOffer["deadline"].get_int64())};
			offer.ExpiryHeight = Height{static_cast<uint64_t>(dbOffer["expiryHeight"].get_int64())};
		}

		void ReadBuyOffers(const bsoncxx::array::view& dbOffers, state::BuyOfferMap& offers) {
			for (const auto& dbOffer : dbOffers) {
				auto doc = dbOffer.get_document().view();
				MosaicId mosaicId;
				state::OfferBase offer;
				ReadOffer(doc, mosaicId, offer);
				auto residualCost = Amount{static_cast<uint64_t>(dbOffer["residualCost"].get_int64())};
				offers.emplace(mosaicId, state::BuyOffer{offer, residualCost});
			}
		}

		void ReadSellOffers(const bsoncxx::array::view& dbOffers, state::SellOfferMap& offers) {
			for (const auto& dbOffer : dbOffers) {
				auto doc = dbOffer.get_document().view();
				MosaicId mosaicId;
				state::OfferBase offer;
				ReadOffer(doc, mosaicId, offer);
				offers.emplace(mosaicId, state::SellOffer{offer});
			}
		}
	}

	state::ExchangeEntry ToExchangeEntry(const bsoncxx::document::view& document) {
		auto dbExchangeEntry = document["exchange"];
		Key owner;
		DbBinaryToModelArray(owner, dbExchangeEntry["owner"].get_binary());
		state::ExchangeEntry entry(owner);

		ReadBuyOffers(dbExchangeEntry["buyOffers"].get_array().value, entry.buyOffers());
		ReadSellOffers(dbExchangeEntry["sellOffers"].get_array().value, entry.sellOffers());

		return entry;
	}

	// endregion
}}}
