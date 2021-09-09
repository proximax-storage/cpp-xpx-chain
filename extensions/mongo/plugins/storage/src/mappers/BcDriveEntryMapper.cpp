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
			for (const auto& modification : activeDataModifications)
				array
					<< bson_stream::open_document
					<< "id" << ToBinary(modification.Id)
					<< "owner" << ToBinary(modification.Owner)
					<< "downloadDataCdi" << ToBinary(modification.DownloadDataCdi)
					<< "uploadSize" << static_cast<int64_t>(modification.UploadSize)
					<< bson_stream::close_document;

			array << bson_stream::close_array;
		}

		void StreamCompletedDataModifications(bson_stream::document& builder, const state::CompletedDataModifications& completedDataModifications) {
			auto array = builder << "completedDataModifications" << bson_stream::open_array;
			for (const auto& modification : completedDataModifications) {
				array
					<< bson_stream::open_document
					<< "id" << ToBinary(modification.Id)
					<< "owner" << ToBinary(modification.Owner)
					<< "downloadDataCdi" << ToBinary(modification.DownloadDataCdi)
					<< "uploadSize" << static_cast<int64_t>(modification.UploadSize)
					<< "state" << utils::to_underlying_type(modification.State)
					<< bson_stream::close_document;
			}

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
				auto uploadSize = static_cast<uint64_t>(doc["uploadSize"].get_int64());

				activeDataModifications.emplace_back(state::ActiveDataModification{ id, owner, downloadDataCdi, uploadSize });
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
				auto uploadSize = static_cast<uint64_t>(doc["uploadSize"].get_int64());
				auto state = static_cast<state::DataModificationState>(static_cast<uint8_t>(doc["state"].get_int32()));

				completedDataModifications.emplace_back(state::CompletedDataModification{ state::ActiveDataModification{ id, owner, downloadDataCdi, uploadSize }, state });
			}
		}

		void ReadVerifications(state::Verifications& verifications, const bsoncxx::array::view& dbVerifications) {
			for (const auto& dbVerification : dbVerifications) {
				auto doc = dbVerification.get_document().view();

				state::Verification verification;
				verification.Height = Height(doc["height"].get_int64());
				DbBinaryToModelArray(verification.VerificationTrigger, doc["verificationTrigger"].get_binary());
				verification.State = static_cast<state::VerificationState>(static_cast<uint8_t>(doc["state"].get_int32()));

				verifications.emplace_back(verification);
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
		ReadVerifications(entry.verifications(), dbDriveEntry["verifications"].get_array().value);

		return entry;
	}

	// endregion
}}}
