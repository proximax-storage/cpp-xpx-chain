/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "BootKeyReplicatorEntryMapper.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	bsoncxx::document::value ToDbModel(const state::BootKeyReplicatorEntry& entry, const Key& key) {
		bson_stream::document builder;
		auto doc = builder << "bootKeyReplicator" << bson_stream::open_document
				<< "nodeBootKey" << ToBinary(entry.nodeBootKey())
				<< "version" << static_cast<int32_t>(entry.version())
				<< "replicatorKey" << ToBinary(entry.replicatorKey());

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	state::BootKeyReplicatorEntry ToBootKeyReplicatorEntry(const bsoncxx::document::view& document) {
		auto dbBootKeyReplicatorEntry = document["bootKeyReplicator"];

		Key nodeBootKey;
		DbBinaryToModelArray(nodeBootKey, dbBootKeyReplicatorEntry["nodeBootKey"].get_binary());

		Key replicatorKey;
		DbBinaryToModelArray(replicatorKey, dbBootKeyReplicatorEntry["replicatorKey"].get_binary());

		state::BootKeyReplicatorEntry entry(nodeBootKey, replicatorKey);
		entry.setVersion(static_cast<VersionType>(dbBootKeyReplicatorEntry["version"].get_int32()));

		return entry;
	}

	// endregion
}}}
