/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CatapultConfigEntryMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "catapult/utils/Casting.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	bsoncxx::document::value ToDbModel(const state::CatapultConfigEntry& entry) {
		bson_stream::document builder;
		auto doc = builder << "catapultConfig" << bson_stream::open_document
				<< "height" << ToInt64(entry.height())
				<< "blockChainConfig" << entry.blockChainConfig()
				<< "supportedEntityVersions" << entry.supportedEntityVersions();

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	state::CatapultConfigEntry ToCatapultConfigEntry(const bsoncxx::document::view& document) {
		state::CatapultConfigEntry entry;
		auto dbCatapultConfigEntry = document["catapultConfig"];
		entry.setHeight(Height{static_cast<uint64_t>(dbCatapultConfigEntry["height"].get_int64())});
		entry.setBlockChainConfig(dbCatapultConfigEntry["blockChainConfig"].get_utf8().value.to_string());
		entry.setSupportedEntityVersions(dbCatapultConfigEntry["supportedEntityVersions"].get_utf8().value.to_string());

		return entry;
	}

	// endregion
}}}
