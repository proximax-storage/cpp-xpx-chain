/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "BlockchainUpgradeEntryMapper.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	bsoncxx::document::value ToDbModel(const state::BlockchainUpgradeEntry& entry) {
		bson_stream::document builder;
		auto doc = builder << "blockchainUpgrade" << bson_stream::open_document
				<< "height" << ToInt64(entry.height())
				<< "blockChainVersion" << ToInt64(entry.blockChainVersion());

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	state::BlockchainUpgradeEntry ToBlockchainUpgradeEntry(const bsoncxx::document::view& document) {
		state::BlockchainUpgradeEntry entry;
		auto dbBlockchainUpgradeEntry = document["blockchainUpgrade"];
		entry.setHeight(Height{static_cast<uint64_t>(dbBlockchainUpgradeEntry["height"].get_int64())});
		entry.setBlockchainVersion(BlockchainVersion{static_cast<uint64_t>(dbBlockchainUpgradeEntry["blockChainVersion"].get_int64())});

		return entry;
	}

	// endregion
}}}
