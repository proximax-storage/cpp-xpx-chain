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

	namespace {
		void StreamDrives(bson_stream::document& builder, const std::vector<Key>& drives) {
			auto array = builder << "drives" << bson_stream::open_array;
			for (const auto& drive : drives)
				array << ToBinary(drive);

			array << bson_stream::close_array;
		}
	}

	bsoncxx::document::value ToDbModel(const state::ReplicatorEntry& entry, const Key& key) {
		bson_stream::document builder;
		auto doc = builder << "replicator" << bson_stream::open_document
				<< "key" << ToBinary(entry.key())
				<< "version" << static_cast<int32_t>(entry.version())
				<< "capacity" << ToInt64(entry.capacity());

		StreamDrives(builder, entry.drives());

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	namespace {
		void ReadDrives(std::vector<Key>& drives, const bsoncxx::array::view& dbDrives) {
			for (const auto& dbDrive : dbDrives) {
				Key drive;
				DbBinaryToModelArray(drive, dbDrive.get_binary());

				drives.push_back(drive);
			}
		}
	}

	// region ToModel

	state::ReplicatorEntry ToReplicatorEntry(const bsoncxx::document::view& document) {

		auto dbReplicatorEntry = document["replicator"];

		Key key;
		DbBinaryToModelArray(key, dbReplicatorEntry["key"].get_binary());
		state::ReplicatorEntry entry(key);

		entry.setVersion(static_cast<VersionType>(dbReplicatorEntry["capacity"].get_int32()));
		entry.setCapacity(Amount(dbReplicatorEntry["capacity"].get_int64()));

		ReadDrives(entry.drives(), dbReplicatorEntry["drives"].get_array().value);

		return entry;
	}

	// endregion
}}}
