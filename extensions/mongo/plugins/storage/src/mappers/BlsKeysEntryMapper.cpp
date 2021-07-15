/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "BlsKeysEntryMapper.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	bsoncxx::document::value ToDbModel(const state::BlsKeysEntry& entry, const BLSPublicKey& blsKey) {
		bson_stream::document builder;
		auto doc = builder << "blsKeyDoc" << bson_stream::open_document
				<< "blsKey" << ToBinary(entry.blsKey())
				<< "version" << static_cast<int32_t>(entry.version())
				<< "key" << ToBinary(entry.key());

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	state::BlsKeysEntry ToBlsKeysEntry(const bsoncxx::document::view& document) {

		auto dbBlsKeysEntry = document["blsKeyDoc"];

		BLSPublicKey blsKey;
		DbBinaryToModelBytes(blsKey, dbBlsKeysEntry["blsKey"].get_binary());
		state::BlsKeysEntry entry(blsKey);

		entry.setVersion(static_cast<VersionType>(dbBlsKeysEntry["version"].get_int32()));

		Key key;
		DbBinaryToModelArray(key, dbBlsKeysEntry["key"].get_binary());
		entry.setKey(key);

		return entry;
	}

	// endregion
}}}
