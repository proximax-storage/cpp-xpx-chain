/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "FileEntryMapper.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	bsoncxx::document::value ToDbModel(const state::FileEntry& entry) {
		bson_stream::document builder;
		auto doc = builder << "file" << bson_stream::open_document
				<< "key" << ToBinary(entry.key())
				<< "parentKey" << ToBinary(entry.parentKey())
				<< "name" << ToBinary(reinterpret_cast<const uint8_t*>(entry.name().data()), entry.name().size());

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	state::FileEntry ToFileEntry(const bsoncxx::document::view& document) {
		auto dbFileEntry = document["file"];
		state::DriveFileKey key;
		DbBinaryToModelArray(key, dbFileEntry["key"].get_binary());
		state::FileEntry entry(key);
		state::DriveFileKey parentKey;
		DbBinaryToModelArray(parentKey, dbFileEntry["parentKey"].get_binary());
		entry.setParentKey(parentKey);
		entry.setName(dbFileEntry["name"].get_utf8().value.to_string());

		return entry;
	}

	// endregion
}}}
