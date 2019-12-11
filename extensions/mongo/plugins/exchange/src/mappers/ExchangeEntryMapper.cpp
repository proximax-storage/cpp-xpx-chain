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
		void StreamOffer(bson_stream::document& builder, const MosaicId& mosaicId, const state::OfferBase& offer) {
			builder
				<< "mosaicId" << ToInt64(mosaicId)
				<< "amount" << ToInt64(offer.Amount)
				<< "initialAmount" << ToInt64(offer.InitialAmount)
				<< "initialCost" << ToInt64(offer.InitialCost)
				<< "deadline" << ToInt64(offer.Deadline)
				<< "price" << offer.price();
		}

		void StreamBuyOffers(bson_stream::document& builder, const state::BuyOfferMap& offers) {
			auto offerArray = builder << "buyOffers" << bson_stream::open_array;
			for (const auto& pair : offers) {
				bson_stream::document offerBuilder;
				StreamOffer(offerBuilder, pair.first, pair.second);
				offerBuilder << "residualCost" << ToInt64(pair.second.ResidualCost);
				offerArray << offerBuilder;
			}
			offerArray << bson_stream::close_array;
		}

		void StreamSellOffers(bson_stream::document& builder, const state::SellOfferMap& offers) {
			auto offerArray = builder << "sellOffers" << bson_stream::open_array;
			for (const auto& pair : offers) {
				bson_stream::document offerBuilder;
				StreamOffer(offerBuilder, pair.first, pair.second);
				offerArray << offerBuilder;
			}
			offerArray << bson_stream::close_array;
		}

		void StreamExpiredBuyOffers(bson_stream::document& builder, const state::ExpiredBuyOfferMap& offers) {
			auto offerArray = builder << "expiredBuyOffers" << bson_stream::open_array;
			for (const auto& pair : offers) {
				bson_stream::document expiredOfferBuilder;
				expiredOfferBuilder << "height" << ToInt64(pair.first);
				StreamBuyOffers(expiredOfferBuilder, pair.second);
				offerArray << expiredOfferBuilder;
			}
			offerArray << bson_stream::close_array;
		}

		void StreamExpiredSellOffers(bson_stream::document& builder, const state::ExpiredSellOfferMap& offers) {
			auto offerArray = builder << "expiredSellOffers" << bson_stream::open_array;
			for (const auto& pair : offers) {
				bson_stream::document expiredOfferBuilder;
				expiredOfferBuilder << "height" << ToInt64(pair.first);
				StreamSellOffers(expiredOfferBuilder, pair.second);
				offerArray << expiredOfferBuilder;
			}
			offerArray << bson_stream::close_array;
		}
	}

	bsoncxx::document::value ToDbModel(const state::ExchangeEntry& entry, const Address& ownerAddress) {
		bson_stream::document builder;
		auto doc = builder << "exchange" << bson_stream::open_document
				<< "owner" << ToBinary(entry.owner())
				<< "ownerAddress" << ToBinary(ownerAddress);

		StreamBuyOffers(builder, entry.buyOffers());
		StreamSellOffers(builder, entry.sellOffers());

		StreamExpiredBuyOffers(builder, entry.expiredBuyOffers());
		StreamExpiredSellOffers(builder, entry.expiredSellOffers());

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

		void ReadExpiredBuyOffers(const bsoncxx::array::view& dbExpiredOffers, state::ExpiredBuyOfferMap& expiredOffers) {
			for (const auto& dbExpiredOffer : dbExpiredOffers) {
				auto doc = dbExpiredOffer.get_document().view();
				auto height = Height{static_cast<uint64_t>(doc["height"].get_int64())};
				state::BuyOfferMap offers;
				ReadBuyOffers(doc["buyOffers"].get_array().value, offers);
				expiredOffers.emplace(height, offers);
			}
		}

		void ReadExpiredSellOffers(const bsoncxx::array::view& dbExpiredOffers, state::ExpiredSellOfferMap& expiredOffers) {
			for (const auto& dbExpiredOffer : dbExpiredOffers) {
				auto doc = dbExpiredOffer.get_document().view();
				auto height = Height{static_cast<uint64_t>(doc["height"].get_int64())};
				state::SellOfferMap offers;
				ReadSellOffers(doc["sellOffers"].get_array().value, offers);
				expiredOffers.emplace(height, offers);
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

		ReadExpiredBuyOffers(dbExchangeEntry["expiredBuyOffers"].get_array().value, entry.expiredBuyOffers());
		ReadExpiredSellOffers(dbExchangeEntry["expiredSellOffers"].get_array().value, entry.expiredSellOffers());

		return entry;
	}

	// endregion
}}}
