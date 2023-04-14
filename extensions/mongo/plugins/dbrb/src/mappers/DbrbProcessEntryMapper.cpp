/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DbrbProcessEntryMapper.h"
#include "catapult/utils/Casting.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	bsoncxx::document::value ToDbModel(const state::DbrbProcessEntry& entry) {
		bson_stream::document builder;
		auto doc = builder << "dbrbProcess" << bson_stream::open_document
				<< "processId" << ToBinary(entry.processId())
				<< "expirationTime" << ToInt64(entry.expirationTime());

		return doc
			<< bson_stream::close_document
			<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	state::DbrbProcessEntry ToDbrbProcessEntry(const bsoncxx::document::view& document) {
		auto dbDbrbProcessEntry = document["dbrbProcess"];

		dbrb::ProcessId processId;
		DbBinaryToModelArray(processId, dbDbrbProcessEntry["processId"].get_binary());
		auto expirationTime = Timestamp(static_cast<uint64_t>(dbDbrbProcessEntry["expirationTime"].get_int64()));

		return state::DbrbProcessEntry(processId, expirationTime);
	}

	// endregion
}}}
