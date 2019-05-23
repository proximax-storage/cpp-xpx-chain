/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CatapultUpgradeEntryMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "catapult/utils/Casting.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	bsoncxx::document::value ToDbModel(const state::CatapultUpgradeEntry& entry) {
		bson_stream::document builder;
		auto doc = builder << "catapultUpgrade" << bson_stream::open_document
				<< "height" << ToInt64(entry.height())
				<< "catapultVersion" << ToInt64(entry.catapultVersion());

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	state::CatapultUpgradeEntry ToCatapultUpgradeEntry(const bsoncxx::document::view& document) {
		state::CatapultUpgradeEntry entry;
		auto dbCatapultUpgradeEntry = document["catapultUpgrade"];
		entry.setHeight(Height{static_cast<uint64_t>(dbCatapultUpgradeEntry["height"].get_int64())});
		entry.setCatapultVersion(CatapultVersion{static_cast<uint64_t>(dbCatapultUpgradeEntry["catapultVersion"].get_int64())});

		return entry;
	}

	// endregion
}}}
