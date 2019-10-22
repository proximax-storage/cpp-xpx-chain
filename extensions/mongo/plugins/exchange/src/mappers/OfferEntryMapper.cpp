/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "OfferEntryMapper.h"
#include "catapult/utils/Casting.h"
#include "src/ExchangeMapperUtils.h"

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	bsoncxx::document::value ToDbModel(const state::OfferEntry& entry) {
		bson_stream::document builder;
		auto doc = builder << "offer" << bson_stream::open_document
				<< "transactionHash" << ToInt32(entry.transactionHash())
				<< "transactionSigner" << ToBinary(entry.transactionSigner())
				<< "offerType" << utils::to_underlying_type(entry.offerType())
				<< "deadline" << ToInt64(entry.deadline())
				<< "expiryHeight" << ToInt64(entry.expiryHeight());
		StreamOffers(builder, "offers", entry.offers());
		StreamOffers(builder, "initialOffers", entry.initialOffers());

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	state::OfferEntry ToOfferEntry(const bsoncxx::document::view& document) {
		auto dbOfferEntry = document["offer"];
		auto transactionHash = utils::ShortHash{static_cast<uint32_t>(dbOfferEntry["transactionHash"].get_int32())};
		Key transactionSigner;
		DbBinaryToModelArray(transactionSigner, dbOfferEntry["transactionSigner"].get_binary());
		auto offerType = static_cast<model::OfferType>(ToUint8(dbOfferEntry["offerType"].get_int32()));
		auto deadline = Height{static_cast<uint64_t>(dbOfferEntry["deadline"].get_int64())};
		state::OfferEntry entry(transactionHash, transactionSigner, offerType, deadline);

		entry.setExpiryHeight(Height{static_cast<uint64_t>(dbOfferEntry["expiryHeight"].get_int64())});

		ReadOffers(dbOfferEntry["offers"].get_array().value, entry.offers());
		ReadOffers(dbOfferEntry["initialOffers"].get_array().value, entry.initialOffers());

		return entry;
	}

	// endregion
}}}
