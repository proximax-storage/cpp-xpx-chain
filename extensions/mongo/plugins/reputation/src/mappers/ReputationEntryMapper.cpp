/**
*** Copyright (c) 2018-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
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
