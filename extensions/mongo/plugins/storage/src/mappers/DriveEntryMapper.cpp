/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DriveEntryMapper.h"
#include "catapult/utils/Casting.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	bsoncxx::document::value ToDbModel(const state::DriveEntry& entry, const Address& accountAddress) {
		bson_stream::document builder;
		auto doc = builder << "drive" << bson_stream::open_document
				<< "multisig" << ToBinary(entry.key())
				<< "multisigAddress" << ToBinary(accountAddress)
				<< "owner" << ToBinary(entry.owner())
				<< "rootHash" << ToBinary(entry.rootHash())
				<< "size" << static_cast<int64_t>(entry.size())
				<< "replicatorCount" << static_cast<int32_t>(entry.replicatorCount());

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	state::DriveEntry ToDriveEntry(const bsoncxx::document::view& document) {

		auto dbDriveEntry = document["drive"];

		Key multisig;
		DbBinaryToModelArray(multisig, dbDriveEntry["multisig"].get_binary());
		state::DriveEntry entry(multisig);

		Key owner;
		DbBinaryToModelArray(owner, dbDriveEntry["owner"].get_binary());
		entry.setOwner(owner);

		Hash256 rootHash;
		DbBinaryToModelArray(rootHash, dbDriveEntry["rootHash"].get_binary());
		entry.setRootHash(rootHash);

		entry.setSize(static_cast<uint64_t>(dbDriveEntry["size"].get_int64()));
		entry.setReplicatorCount(static_cast<uint16_t>(dbDriveEntry["replicatorCount"].get_int32()));

		return entry;
	}

	// endregion
}}}
