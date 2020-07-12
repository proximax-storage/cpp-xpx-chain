/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NetworkConfigEntryMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "catapult/utils/Casting.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	bsoncxx::document::value ToDbModel(const state::NetworkConfigEntry& entry) {
		bson_stream::document builder;
		auto doc = builder << "networkConfig" << bson_stream::open_document
				<< "height" << ToInt64(entry.height())
				<< "networkConfig" << entry.networkConfig()
				<< "supportedEntityVersions" << entry.supportedEntityVersions();

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	state::NetworkConfigEntry ToNetworkConfigEntry(const bsoncxx::document::view& document) {
		state::NetworkConfigEntry entry;
		auto dbNetworkConfigEntry = document["networkConfig"];
		entry.setHeight(Height{static_cast<uint64_t>(dbNetworkConfigEntry["height"].get_int64())});
		entry.setBlockChainConfig(std::string(dbNetworkConfigEntry["networkConfig"].get_utf8().value));
		entry.setSupportedEntityVersions(std::string(dbNetworkConfigEntry["supportedEntityVersions"].get_utf8().value));

		return entry;
	}

	// endregion
}}}
