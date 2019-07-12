/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ReputationEntryMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "catapult/utils/Casting.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	bsoncxx::document::value ToDbModel(const state::ReputationEntry& entry, const Address& accountAddress) {
		bson_stream::document builder;
		auto doc = builder << "reputation" << bson_stream::open_document
				<< "account" << ToBinary(entry.key())
				<< "accountAddress" << ToBinary(accountAddress)
				<< "positiveInteractions" << static_cast<int64_t>(entry.positiveInteractions().unwrap())
				<< "negativeInteractions" << static_cast<int64_t>(entry.negativeInteractions().unwrap());

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	state::ReputationEntry ToReputationEntry(const bsoncxx::document::view& document) {
		auto dbReputationEntry = document["reputation"];
		Key account;
		DbBinaryToModelArray(account, dbReputationEntry["account"].get_binary());
		state::ReputationEntry entry(account);

		auto positiveInteractions = static_cast<uint64_t>(dbReputationEntry["positiveInteractions"].get_int64());
		auto negativeInteractions = static_cast<uint64_t>(dbReputationEntry["negativeInteractions"].get_int64());
		entry.setPositiveInteractions(Reputation{positiveInteractions});
		entry.setNegativeInteractions(Reputation{negativeInteractions});

		return entry;
	}

	// endregion
}}}
