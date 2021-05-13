/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ReplicatorEntryMapper.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	bsoncxx::document::value ToDbModel(const state::ReplicatorEntry& entry, const Key& key) {
		bson_stream::document builder;
		auto doc = builder << "replicator" << bson_stream::open_document
				<< "key" << ToBinary(entry.key())
				<< "capacity" << ToInt64(entry.capacity());

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	state::ReplicatorEntry ToReplicatorEntry(const bsoncxx::document::view& document) {

		auto dbReplicatorEntry = document["replicator"];

		Key key;
		DbBinaryToModelArray(key, dbReplicatorEntry["key"].get_binary());
		state::ReplicatorEntry entry(key);

		entry.setCapacity(Amount(dbReplicatorEntry["capacity"].get_int64()));

		return entry;
	}

	// endregion
}}}
