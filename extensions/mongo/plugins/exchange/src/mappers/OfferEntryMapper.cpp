/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "OfferEntryMapper.h"
#include "src/ExchangeMapperUtils.h"

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	bsoncxx::document::value ToDbModel(const state::OfferEntry& entry) {
		bson_stream::document builder;
		auto doc = builder << "buyOffer" << bson_stream::open_document
				<< "transactionHash" << ToInt32(entry.transactionHash())
				<< "transactionSigner" << ToBinary(entry.transactionSigner())
				<< "deadline" << ToInt64(entry.deadline())
				<< "deposit" << ToInt64(entry.deposit());
		StreamOffers(builder, entry.offers());

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	state::OfferEntry ToOfferEntry(const bsoncxx::document::view& document) {
		auto dbOfferEntry = document["buyOffer"];
		auto transactionHash = utils::ShortHash{static_cast<uint32_t>(dbOfferEntry["transactionHash"].get_int32())};
		Key transactionSigner;
		DbBinaryToModelArray(transactionSigner, dbOfferEntry["transactionSigner"].get_binary());
		state::OfferEntry entry(transactionHash, transactionSigner);

		entry.setDeadline(Timestamp{static_cast<uint64_t>(dbOfferEntry["deadline"].get_int64())});
		entry.setDeposit(Amount{static_cast<uint64_t>(dbOfferEntry["deposit"].get_int64())});

		ReadOffers(dbOfferEntry["offers"].get_array().value, entry.offers());

		return entry;
	}

	// endregion
}}}
