/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "BcDriveEntryMapper.h"
#include "catapult/utils/Casting.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	namespace {
		void StreamActiveDataModifications(bson_stream::document& builder, const state::ActiveDataModifications& activeDataModifications) {
			auto array = builder << "activeDataModifications" << bson_stream::open_array;
			for (const auto& modification : activeDataModifications) {
				auto pFolderName = (const uint8_t*) (modification.FolderName.c_str());
				array
						<< bson_stream::open_document
						<< "id" << ToBinary(modification.Id)
						<< "owner" << ToBinary(modification.Owner)
						<< "downloadDataCdi" << ToBinary(modification.DownloadDataCdi)
						<< "expectedUploadSize" << static_cast<int64_t>(modification.ExpectedUploadSize)
						<< "actualUploadSize" << static_cast<int64_t>(modification.ActualUploadSize)
						<< "folderName" << ToBinary(pFolderName, modification.FolderName.size())
						<< "readyForApproval" << modification.ReadyForApproval
						<< bson_stream::close_document;
			}

			array << bson_stream::close_array;
		}

		void StreamCompletedDataModifications(bson_stream::document& builder, const state::CompletedDataModifications& completedDataModifications) {
			auto array = builder << "completedDataModifications" << bson_stream::open_array;
			for (const auto& modification : completedDataModifications) {
				auto pFolderName = (const uint8_t*) (modification.FolderName.c_str());
				array
						<< bson_stream::open_document
						<< "id" << ToBinary(modification.Id)
						<< "owner" << ToBinary(modification.Owner)
						<< "downloadDataCdi" << ToBinary(modification.DownloadDataCdi)
						<< "expectedUploadSize" << static_cast<int64_t>(modification.ExpectedUploadSize)
						<< "actualUploadSize" << static_cast<int64_t>(modification.ActualUploadSize)
						<< "folderName" << ToBinary(pFolderName, modification.FolderName.size())
						<< "readyForApproval" << modification.ReadyForApproval
						<< "state" << utils::to_underlying_type(modification.State)
						<< bson_stream::close_document;
			}

			array << bson_stream::close_array;
		}

		void StreamConfirmedUsedSizes(bson_stream::document& builder, const state::UsedSizeMap& confirmedUsedSizes) {
			auto array = builder << "confirmedUsedSizes" << bson_stream::open_array;
			for (const auto& pair : confirmedUsedSizes)
				array
					<< bson_stream::open_document
					<< "replicator" << ToBinary(pair.first)
					<< "size" << static_cast<int64_t>(pair.second)
					<< bson_stream::close_document;

			array << bson_stream::close_array;
		}

		void StreamReplicators(bson_stream::document& builder, const utils::KeySet& replicators) {
			auto array = builder << "replicators" << bson_stream::open_array;
			for (const auto& replicatorKey : replicators)
				array << ToBinary(replicatorKey);
			array << bson_stream::close_array;
		}
	}

	bsoncxx::document::value ToDbModel(const state::BcDriveEntry& entry, const Address& accountAddress) {
		bson_stream::document builder;
		auto doc = builder << "drive" << bson_stream::open_document
						   << "multisig" << ToBinary(entry.key())
						   << "multisigAddress" << ToBinary(accountAddress)
						   << "owner" << ToBinary(entry.owner())
						   << "rootHash" << ToBinary(entry.rootHash())
						   << "size" << static_cast<int64_t>(entry.size())
						   << "usedSize" << static_cast<int64_t>(entry.usedSize())
						   << "metaFilesSize" << static_cast<int64_t>(entry.metaFilesSize())
						   << "replicatorCount" << static_cast<int32_t>(entry.replicatorCount());

		StreamActiveDataModifications(builder, entry.activeDataModifications());
		StreamCompletedDataModifications(builder, entry.completedDataModifications());
		StreamConfirmedUsedSizes(builder, entry.confirmedUsedSizes());
		StreamReplicators(builder, entry.replicators());

		return doc
			   << bson_stream::close_document
			   << bson_stream::finalize;
	}

	// endregion

	// region ToModel

	namespace {
		void ReadActiveDataModifications(state::ActiveDataModifications& activeDataModifications, const bsoncxx::array::view& dbActiveDataModifications) {
			for (const auto& dbModification : dbActiveDataModifications) {
				auto doc = dbModification.get_document().view();

				Hash256 id;
				DbBinaryToModelArray(id, doc["id"].get_binary());
				Key owner;
				DbBinaryToModelArray(owner, doc["owner"].get_binary());
				Hash256 downloadDataCdi;
				DbBinaryToModelArray(downloadDataCdi, doc["downloadDataCdi"].get_binary());
				auto expectedUploadSize = static_cast<uint64_t>(doc["expectedUploadSize"].get_int64());
				auto actualUploadSize = static_cast<uint64_t>(doc["actualUploadSize"].get_int64());
				auto binaryFolderName = doc["folderName"].get_binary();
				std::string folderName((const char*) binaryFolderName.bytes, binaryFolderName.size);
				auto readyForApproval = doc["readyForApproval"].get_bool();
				activeDataModifications.emplace_back(state::ActiveDataModification(id, owner, downloadDataCdi, expectedUploadSize, actualUploadSize, folderName, readyForApproval));
			}
		}

		void ReadCompletedDataModifications(state::CompletedDataModifications& completedDataModifications, const bsoncxx::array::view& dbCompletedDataModifications) {
			for (const auto& dbModification : dbCompletedDataModifications) {
				auto doc = dbModification.get_document().view();

				Hash256 id;
				DbBinaryToModelArray(id, doc["id"].get_binary());
				Key owner;
				DbBinaryToModelArray(owner, doc["owner"].get_binary());
				Hash256 downloadDataCdi;
				DbBinaryToModelArray(downloadDataCdi, doc["downloadDataCdi"].get_binary());
				auto expectedUploadSize = static_cast<uint64_t>(doc["expectedUploadSize"].get_int64());
				auto actualUploadSize = static_cast<uint64_t>(doc["actualUploadSize"].get_int64());
				auto binaryFolderName = doc["folderName"].get_binary();
				std::string folderName((const char*) binaryFolderName.bytes, binaryFolderName.size);
				bool readyForApproval = doc["readyForApproval"].get_bool();
				auto state = static_cast<state::DataModificationState>(static_cast<uint8_t>(doc["state"].get_int32()));

				completedDataModifications.emplace_back(state::CompletedDataModification{ state::ActiveDataModification(id, owner, downloadDataCdi, expectedUploadSize, actualUploadSize, folderName, readyForApproval), state });
			}
		}

		void ReadConfirmedUsedSizes(state::UsedSizeMap& confirmedUsedSizes, const bsoncxx::array::view& dbConfirmedUsedSizes) {
			for (const auto& dbConfirmedUsedSize : dbConfirmedUsedSizes) {
				auto doc = dbConfirmedUsedSize.get_document().view();

				Key replicatorKey;
				DbBinaryToModelArray(replicatorKey, doc["replicator"].get_binary());
				auto size = static_cast<uint64_t>(doc["size"].get_int64());

				confirmedUsedSizes.emplace(replicatorKey, size);
			}
		}

		void ReadReplicators(utils::KeySet& replicators, const bsoncxx::array::view& dbReplicators) {
			for (const auto& dbReplicator : dbReplicators) {
				Key replicatorKey;
				DbBinaryToModelArray(replicatorKey, dbReplicator.get_binary());
				replicators.insert(replicatorKey);
			}
		}
	}

	state::BcDriveEntry ToDriveEntry(const bsoncxx::document::view& document) {

		auto dbDriveEntry = document["drive"];

		Key multisig;
		DbBinaryToModelArray(multisig, dbDriveEntry["multisig"].get_binary());
		state::BcDriveEntry entry(multisig);

		Key owner;
		DbBinaryToModelArray(owner, dbDriveEntry["owner"].get_binary());
		entry.setOwner(owner);

		Hash256 rootHash;
		DbBinaryToModelArray(rootHash, dbDriveEntry["rootHash"].get_binary());
		entry.setRootHash(rootHash);

		entry.setSize(static_cast<uint64_t>(dbDriveEntry["size"].get_int64()));
		entry.setUsedSize(static_cast<uint64_t>(dbDriveEntry["usedSize"].get_int64()));
		entry.setMetaFilesSize(static_cast<uint64_t>(dbDriveEntry["metaFilesSize"].get_int64()));
		entry.setReplicatorCount(static_cast<uint16_t>(dbDriveEntry["replicatorCount"].get_int32()));

		ReadActiveDataModifications(entry.activeDataModifications(), dbDriveEntry["activeDataModifications"].get_array().value);
		ReadCompletedDataModifications(entry.completedDataModifications(), dbDriveEntry["completedDataModifications"].get_array().value);
		ReadConfirmedUsedSizes(entry.confirmedUsedSizes(), dbDriveEntry["confirmedUsedSizes"].get_array().value);
		ReadReplicators(entry.replicators(), dbDriveEntry["replicators"].get_array().value);

		return entry;
	}

	// endregion
}}}
