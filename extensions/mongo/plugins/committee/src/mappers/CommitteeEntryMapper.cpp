/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommitteeEntryMapper.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	bsoncxx::document::value ToDbModel(const state::CommitteeEntry& entry, const Address& address) {
		bson_stream::document builder;
		auto doc = builder << "harvester" << bson_stream::open_document
				<< "key" << ToBinary(entry.key())
				<< "address" << ToBinary(address)
				<< "lastSigningBlockHeight" << ToInt64(entry.lastSigningBlockHeight())
				<< "effectiveBalance" << ToInt64(entry.effectiveBalance())
				<< "canHarvest" << entry.canHarvest()
				<< "activity" << entry.activity()
				<< "greed" << entry.greed();

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	state::CommitteeEntry ToCommitteeEntry(const bsoncxx::document::view& document) {
		auto dbCommitteeEntry = document["harvester"];
		Key key;
		DbBinaryToModelArray(key, dbCommitteeEntry["key"].get_binary());
		auto lastSigningBlockHeight = GetValue64<Height>(dbCommitteeEntry["lastSigningBlockHeight"]);
		auto effectiveBalance = GetValue64<Importance>(dbCommitteeEntry["effectiveBalance"]);
		auto canHarvest = dbCommitteeEntry["canHarvest"].get_bool().value;
		auto activity = dbCommitteeEntry["activity"].get_double().value;
		auto greed = dbCommitteeEntry["greed"].get_double().value;

		return state::CommitteeEntry(key, lastSigningBlockHeight, effectiveBalance, canHarvest, activity, greed);
	}

	// endregion
}}}
