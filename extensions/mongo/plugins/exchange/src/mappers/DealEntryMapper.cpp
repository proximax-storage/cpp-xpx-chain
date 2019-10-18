/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DealEntryMapper.h"
#include "src/ExchangeMapperUtils.h"

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	namespace {
		bson_stream::array_context& StreamDeposit(bson_stream::array_context& context, const utils::ShortHash& hash, const Amount& deposit) {
			context
				<< bson_stream::open_document
				<< "transactionHash" << ToInt32(hash)
				<< "deposit" << ToInt64(deposit)
				<< bson_stream::close_document;
			return context;
		}

		void StreamDeposits(bson_stream::document &builder, const state::DealEntry::DepositMap& deposits) {
			auto array = builder << "deposits" << bson_stream::open_array;
			for (const auto& pair : deposits) {
				StreamDeposit(array, pair.first, pair.second);
			}
			array << bson_stream::close_array;
		}

		void StreamSuggestedDeals(bson_stream::document &builder, const state::OfferMap& offers) {
			auto array = builder << "suggestedDeals" << bson_stream::open_array;
			for (const auto& pair : offers) {
				StreamOffer(array, pair.second);
			}
			array << bson_stream::close_array;
		}

		void StreamAcceptedDeal(bson_stream::document& builder, const utils::ShortHash& hash, const state::OfferMap& offers) {
			builder << "transactionHash" << ToInt32(hash);
			auto array = builder << "acceptedOffers" << bson_stream::open_array;
			for (const auto& pair : offers) {
				StreamOffer(array, pair.second);
			}

			array << bson_stream::close_array;
		}

		void StreamAcceptedDeals(bson_stream::document &builder, const state::DealEntry::AcceptedDeals& acceptedDeals) {
			auto array = builder << "acceptedDeals" << bson_stream::open_array;
			for (const auto& pair : acceptedDeals) {
				bson_stream::document acceptedDealBuilder;
				StreamAcceptedDeal(acceptedDealBuilder, pair.first, pair.second);
				array << acceptedDealBuilder;
			}
			array << bson_stream::close_array;
		}
	}

	bsoncxx::document::value ToDbModel(const state::DealEntry& entry) {
		bson_stream::document builder;
		auto doc = builder << "buyOffer" << bson_stream::open_document
				<< "transactionHash" << ToInt32(entry.transactionHash())
				<< "deposit" << ToInt64(entry.deposit());
		StreamDeposits(builder, entry.deposits());
		StreamSuggestedDeals(builder, entry.suggestedDeals());
		StreamAcceptedDeals(builder, entry.acceptedDeals());

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	namespace {
		void ReadOffers(const bsoncxx::array::view& dbOffers, state::OfferMap& offers) {
			for (const auto& dbOffer : dbOffers) {
				auto doc = dbOffer.get_document().view();
				auto mosaicId = UnresolvedMosaicId{static_cast<uint64_t>(doc["mosaicId"].get_int64())};
				auto mosaicAmount = Amount{static_cast<uint64_t>(doc["mosaicAmount"].get_int64())};
				auto cost = Amount{static_cast<uint64_t>(doc["mosaicId"].get_int64())};

				offers.emplace(mosaicId, model::Offer{model::UnresolvedMosaic{mosaicId, mosaicAmount}, cost});
			}
		}

		void ReadAcceptedDeals(const bsoncxx::array::view& dbAcceptedDeals, state::DealEntry::AcceptedDeals& acceptedDeals) {
			for (const auto& dbAcceptedDeal : dbAcceptedDeals) {
				auto doc = dbAcceptedDeal.get_document().view();
				auto transactionHash = utils::ShortHash{static_cast<uint32_t>(doc["transactionHash"].get_int32())};
				state::OfferMap acceptedOffers;
				ReadOffers(doc["acceptedOffers"].get_array().value, acceptedOffers);
				acceptedDeals.emplace(transactionHash, acceptedOffers);
			}
		}
	}

	state::DealEntry ToDealEntry(const bsoncxx::document::view& document) {
		auto dbDealEntry = document["buyOffer"];
		auto transactionHash = utils::ShortHash{static_cast<uint32_t>(dbDealEntry["transactionHash"].get_int32())};
		state::DealEntry entry(transactionHash);

		entry.setDeposit(Amount{static_cast<uint64_t>(dbDealEntry["deposit"].get_int64())});

		auto dbDeposits = dbDealEntry["deposits"].get_array().value;
		for (const auto& dbDeposit : dbDeposits) {
			transactionHash = utils::ShortHash{static_cast<uint32_t>(dbDealEntry["transactionHash"].get_int32())};
			auto deposit = Amount{static_cast<uint64_t>(dbDeposit["deposit"].get_int64())};

			entry.deposits().emplace(transactionHash, deposit);
		}

		ReadOffers(dbDealEntry["suggestedDeals"].get_array().value, entry.suggestedDeals());

		ReadAcceptedDeals(dbDealEntry["acceptedDeals"].get_array().value, entry.acceptedDeals());

		return entry;
	}

	// endregion
}}}
