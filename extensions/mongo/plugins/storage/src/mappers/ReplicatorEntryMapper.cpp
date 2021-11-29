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
		void StreamDrives(bson_stream::document& builder, const state::DrivesMap& drives) {
			auto array = builder << "drives" << bson_stream::open_array;
			for (const auto& drivePair : drives) {
				bson_stream::document driveBuilder;
				driveBuilder
						<< "drive" << ToBinary(drivePair.first)
						<< "lastApprovedDataModificationId" << ToBinary(drivePair.second.LastApprovedDataModificationId)
						<< "dataModificationIdIsValid" << drivePair.second.DataModificationIdIsValid	// TODO: Double-check if streamed correctly
						<< "initialDownloadWork" << static_cast<int64_t>(drivePair.second.InitialDownloadWork)
						<< "lastCompletedCumulativeDownloadWork" << static_cast<int64_t>(drivePair.second.LastCompletedCumulativeDownloadWork);
				array << driveBuilder;
			}

			array << bson_stream::close_array;
		}
	}

	bsoncxx::document::value ToDbModel(const state::ReplicatorEntry& entry, const Key& key) {
		bson_stream::document builder;
		auto doc = builder << "replicator" << bson_stream::open_document
				<< "key" << ToBinary(entry.key())
				<< "version" << static_cast<int32_t>(entry.version())
				<< "capacity" << ToInt64(entry.capacity())
				<< "blsKey" << ToBinary(entry.blsKey());

		StreamDrives(builder, entry.drives());

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	namespace {
		void ReadDrives(state::DrivesMap& drives, const bsoncxx::array::view& dbDrives) {
			for (const auto& dbDrive : dbDrives) {
				auto doc = dbDrive.get_document().view();

				Key drive;
				DbBinaryToModelArray(drive, doc["drive"].get_binary());

				state::DriveInfo info;
				DbBinaryToModelArray(info.LastApprovedDataModificationId, doc["lastApprovedDataModificationId"].get_binary());
				info.DataModificationIdIsValid = doc["dataModificationIdIsValid"].get_bool();	// TODO: Double-check if read correctly
				info.InitialDownloadWork = doc["initialDownloadWork"].get_int64();
				info.LastCompletedCumulativeDownloadWork = doc["lastCompletedCumulativeDownloadWork"].get_int64();

				drives.emplace(drive, info);
			}
		}
	}

	// region ToModel

	state::ReplicatorEntry ToReplicatorEntry(const bsoncxx::document::view& document) {

		auto dbReplicatorEntry = document["replicator"];

		Key key;
		DbBinaryToModelArray(key, dbReplicatorEntry["key"].get_binary());
		state::ReplicatorEntry entry(key);

		entry.setVersion(static_cast<VersionType>(dbReplicatorEntry["capacity"].get_int32()));	// TODO: dbReplicatorEntry["version"]
		entry.setCapacity(Amount(dbReplicatorEntry["capacity"].get_int64()));

		BLSPublicKey blsKey;
		DbBinaryToModelBytes(blsKey, dbReplicatorEntry["blsKey"].get_binary());
		entry.setBlsKey(blsKey);

		ReadDrives(entry.drives(), dbReplicatorEntry["drives"].get_array().value);

		return entry;
	}

	// endregion
}}}
