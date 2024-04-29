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
						<< "initialDownloadWork" << static_cast<int64_t>(drivePair.second.InitialDownloadWorkMegabytes)
						<< "lastCompletedCumulativeDownloadWork" << static_cast<int64_t>(drivePair.second.LastCompletedCumulativeDownloadWorkBytes);
				array << driveBuilder;
			}

			array << bson_stream::close_array;
		}

		void StreamDownloadChannels(bson_stream::document& builder, const std::set<Hash256>& downloadChannels) {
			auto array = builder << "downloadChannels" << bson_stream::open_array;
			for (const auto& downloadChannelId : downloadChannels)
				array << ToBinary(downloadChannelId);
			array << bson_stream::close_array;
		}
	}

	bsoncxx::document::value ToDbModel(const state::ReplicatorEntry& entry, const Key& key) {
		bson_stream::document builder;
		auto doc = builder << "replicator" << bson_stream::open_document
				<< "key" << ToBinary(entry.key())
				<< "version" << static_cast<int32_t>(entry.version())
				<< "nodeBootKey" << ToBinary(entry.nodeBootKey());

		StreamDrives(builder, entry.drives());
		StreamDownloadChannels(builder, entry.downloadChannels());

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
				info.InitialDownloadWorkMegabytes = doc["initialDownloadWork"].get_int64();
				info.LastCompletedCumulativeDownloadWorkBytes = doc["lastCompletedCumulativeDownloadWork"].get_int64();

				drives.emplace(drive, info);
			}
		}

		void ReadDownloadChannels(std::set<Hash256>& downloadChannels, const bsoncxx::array::view& dbDownloadChannels) {
			for (const auto& dbDownloadChannelId : dbDownloadChannels) {
				Hash256 downloadChannelId;
				DbBinaryToModelArray(downloadChannelId, dbDownloadChannelId.get_binary());
				downloadChannels.emplace(std::move(downloadChannelId));
			}
		}
	}

	// region ToModel

	state::ReplicatorEntry ToReplicatorEntry(const bsoncxx::document::view& document) {

		auto dbReplicatorEntry = document["replicator"];

		Key key;
		DbBinaryToModelArray(key, dbReplicatorEntry["key"].get_binary());
		state::ReplicatorEntry entry(key);

		Key nodeBootKey;
		DbBinaryToModelArray(nodeBootKey, dbReplicatorEntry["nodeBootKey"].get_binary());
		entry.setNodeBootKey(nodeBootKey);

		entry.setVersion(static_cast<VersionType>(dbReplicatorEntry["version"].get_int32()));

		ReadDrives(entry.drives(), dbReplicatorEntry["drives"].get_array().value);
		ReadDownloadChannels(entry.downloadChannels(), dbReplicatorEntry["downloadChannels"].get_array().value);

		return entry;
	}

	// endregion
}}}
