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
						<< "expectedUploadSize" << static_cast<int64_t>(modification.ExpectedUploadSizeMegabytes)
						<< "actualUploadSize" << static_cast<int64_t>(modification.ActualUploadSizeMegabytes)
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
						<< "expectedUploadSize" << static_cast<int64_t>(modification.ExpectedUploadSizeMegabytes)
						<< "actualUploadSize" << static_cast<int64_t>(modification.ActualUploadSizeMegabytes)
						<< "folderName" << ToBinary(pFolderName, modification.FolderName.size())
						<< "readyForApproval" << modification.ReadyForApproval
						<< "state" << utils::to_underlying_type(modification.ApprovalState)
						<< "success" << static_cast<int32_t>(modification.SuccessState)
						<< bson_stream::close_document;
			}

			array << bson_stream::close_array;
		}

		void StreamConfirmedUsedSizes(bson_stream::document& builder, const state::SizeMap& confirmedUsedSizes) {
			auto array = builder << "confirmedUsedSizes" << bson_stream::open_array;
			for (const auto& pair : confirmedUsedSizes)
				array
					<< bson_stream::open_document
					<< "replicator" << ToBinary(pair.first)
					<< "size" << static_cast<int64_t>(pair.second)
					<< bson_stream::close_document;

			array << bson_stream::close_array;
		}

		template<class TContainer>
		void StreamReplicators(std::string&& arrayName, bson_stream::document& builder, const TContainer& replicators) {
			auto array = builder << arrayName << bson_stream::open_array;
			for (const auto& replicatorKey : replicators)
				array << ToBinary(replicatorKey);
			array << bson_stream::close_array;
		}

		void StreamShards(bson_stream::document& builder, const state::Shards& shards) {
			auto array = builder << "shards" << bson_stream::open_array;
			for (int32_t i = 0u; i < shards.size(); ++i) {
				bson_stream::document shardBuilder;
				shardBuilder << "id" << i;
				StreamReplicators("replicators", shardBuilder, shards[i]);
				array << shardBuilder;
			}

			array << bson_stream::close_array;
		}

		void StreamVerification(bson_stream::document& builder, const std::optional<state::Verification>& verification) {
			if (verification) {
				builder << "verification" << bson_stream::open_document
					<< "verificationTrigger" << ToBinary(verification->VerificationTrigger)
					<< "expiration" << ToInt64(verification->Expiration)
					<< "duration" << static_cast<int32_t>(verification->Duration);
					StreamShards(builder, verification->Shards);
					builder << bson_stream::close_document;
			}
		}

		void StreamDownloadShards(bson_stream::document& builder, const state::DownloadShards& downloadShards) {
			auto array = builder << "downloadShards" << bson_stream::open_array;
			for (const auto& id : downloadShards) {
				bson_stream::document shardBuilder;
				shardBuilder << "downloadChannelId" << ToBinary(id);
				array << shardBuilder;
			}

			array << bson_stream::close_array;
		}

		void StreamUploadInfo(const std::string& arrayName, bson_stream::document& builder, const std::map<Key, uint64_t>& info) {
			auto array = builder << arrayName << bson_stream::open_array;
			for (const auto& [replicatorKey, uploadSize] : info)
				array << bson_stream::open_document
					  << "key" << ToBinary(replicatorKey)
					  << "uploadSize" << static_cast<int64_t>(uploadSize)
					  << bson_stream::close_document;
			array << bson_stream::close_array;
		}

		void StreamModificationShards(bson_stream::document& builder, const state::ModificationShards& dataModificationShards) {
			auto array = builder << "dataModificationShards" << bson_stream::open_array;
			for (const auto& pair : dataModificationShards) {
				bson_stream::document shardBuilder;
				shardBuilder << "replicator" << ToBinary(pair.first);
				StreamUploadInfo("actualShardReplicators", shardBuilder, pair.second.ActualShardMembers);
				StreamUploadInfo("formerShardReplicators", shardBuilder, pair.second.FormerShardMembers);
				shardBuilder << "ownerUpload" << static_cast<int64_t>(pair.second.OwnerUpload);
				array << shardBuilder;
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
				           << "usedSizeBytes" << static_cast<int64_t>(entry.usedSizeBytes())
				           << "metaFilesSizeBytes" << static_cast<int64_t>(entry.metaFilesSizeBytes())
				           << "replicatorCount" << static_cast<int32_t>(entry.replicatorCount())
						   << "ownerManagement" << static_cast<int32_t>(entry.ownerManagement());

		StreamActiveDataModifications(builder, entry.activeDataModifications());
		StreamCompletedDataModifications(builder, entry.completedDataModifications());
		StreamConfirmedUsedSizes(builder, entry.confirmedUsedSizes());
		StreamReplicators("replicators", builder, entry.replicators());
		StreamReplicators("offboardingReplicators", builder, entry.offboardingReplicators());
		StreamVerification(builder, entry.verification());
		StreamDownloadShards(builder, entry.downloadShards());
		StreamModificationShards(builder, entry.dataModificationShards());

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
				auto state = static_cast<state::DataModificationApprovalState>(static_cast<uint8_t>(doc["state"].get_int32()));
				auto success = static_cast<uint8_t>(doc["success"].get_int32());
				completedDataModifications.emplace_back(state::CompletedDataModification{ state::ActiveDataModification(id, owner, downloadDataCdi, expectedUploadSize, actualUploadSize, folderName, readyForApproval), state, success });
			}
		}

		void ReadConfirmedUsedSizes(state::SizeMap& confirmedUsedSizes, const bsoncxx::array::view& dbConfirmedUsedSizes) {
			for (const auto& dbConfirmedUsedSize : dbConfirmedUsedSizes) {
				auto doc = dbConfirmedUsedSize.get_document().view();

				Key replicatorKey;
				DbBinaryToModelArray(replicatorKey, doc["replicator"].get_binary());
				auto size = static_cast<uint64_t>(doc["size"].get_int64());

				confirmedUsedSizes.emplace(replicatorKey, size);
			}
		}

		void ReadReplicators(utils::SortedKeySet& replicators, const bsoncxx::array::view& dbReplicators) {
			for (const auto& dbReplicator : dbReplicators) {
				Key replicatorKey;
				DbBinaryToModelArray(replicatorKey, dbReplicator.get_binary());
				replicators.emplace(std::move(replicatorKey));
			}
		}

		void ReadReplicators(std::vector<Key>& replicators, const bsoncxx::array::view& dbReplicators) {
			replicators.reserve(dbReplicators.length());
			for (const auto& dbReplicator : dbReplicators) {
				Key replicatorKey;
				DbBinaryToModelArray(replicatorKey, dbReplicator.get_binary());
				replicators.emplace_back(std::move(replicatorKey));
			}
		}

		void ReadShards(state::Shards& shards, const bsoncxx::array::view& dbShards) {
			shards.reserve(dbShards.length());
			for (const auto& dbShard : dbShards) {
				shards.emplace_back();
				ReadReplicators(shards.back(), dbShard["replicators"].get_array().value);
			}
		}

		void ReadVerification(std::optional<state::Verification>& verification, const bsoncxx::document::view& dbVerification) {
			verification = state::Verification();
			DbBinaryToModelArray(verification->VerificationTrigger, dbVerification["verificationTrigger"].get_binary());
			verification->Expiration = Timestamp(static_cast<uint64_t>(dbVerification["expiration"].get_int64()));
			verification->Duration = dbVerification["duration"].get_int32();
			ReadShards(verification->Shards, dbVerification["shards"].get_array().value);
		}

		void ReadDownloadShards(state::DownloadShards& downloadShards, const bsoncxx::array::view& dbDownloadShards) {
			for (const auto& dbShard : dbDownloadShards) {
				auto doc = dbShard.get_document().view();
				Hash256 downloadChannelId;
				DbBinaryToModelArray(downloadChannelId, doc["downloadChannelId"].get_binary());
				downloadShards.emplace(std::move(downloadChannelId));
			}
		}

		void ReadUploadInfo(std::map<Key, uint64_t>& shard, const bsoncxx::array::view& dbShard) {
			for (const auto& replicator: dbShard) {
				auto doc = replicator.get_document().view();
				Key replicatorKey;
				DbBinaryToModelArray(replicatorKey, doc["key"].get_binary());
				uint64_t uploadSize = static_cast<uint64_t>(doc["uploadSize"].get_int64());
				shard[replicatorKey] = uploadSize;
			}
		}

		void ReadModificationShards(state::ModificationShards& dataModificationShards, const bsoncxx::array::view& dbModificationShards) {
			for (const auto& dbShard : dbModificationShards) {
				auto doc = dbShard.get_document().view();
				Key replicatorKey;
				DbBinaryToModelArray(replicatorKey, doc["replicator"].get_binary());
				auto& shardsPair = dataModificationShards[replicatorKey];
//				ReadUploadInfo(shardsPair.m_actualShardMembers, doc["actualShardReplicators"].get_array().value);
//				ReadUploadInfo(shardsPair.m_formerShardMembers, doc["formerShardReplicators"].get_array().value);
				shardsPair.OwnerUpload = static_cast<uint64_t>(doc["ownerUpload"].get_int64());
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
		entry.setUsedSizeBytes(static_cast<uint64_t>(dbDriveEntry["usedSizeBytes"].get_int64()));
		entry.setMetaFilesSizeBytes(static_cast<uint64_t>(dbDriveEntry["metaFilesSizeBytes"].get_int64()));
		entry.setReplicatorCount(static_cast<uint16_t>(dbDriveEntry["replicatorCount"].get_int32()));
		entry.setOwnerManagement(static_cast<state::OwnerManagement>(static_cast<uint8_t>(dbDriveEntry["ownerManagement"].get_int32())));

		ReadActiveDataModifications(entry.activeDataModifications(), dbDriveEntry["activeDataModifications"].get_array().value);
		ReadCompletedDataModifications(entry.completedDataModifications(), dbDriveEntry["completedDataModifications"].get_array().value);
		ReadConfirmedUsedSizes(entry.confirmedUsedSizes(), dbDriveEntry["confirmedUsedSizes"].get_array().value);
		ReadReplicators(entry.replicators(), dbDriveEntry["replicators"].get_array().value);
		ReadReplicators(entry.offboardingReplicators(), dbDriveEntry["offboardingReplicators"].get_array().value);

		auto verificationElement = dbDriveEntry["verification"];
		if (verificationElement) {
			ReadVerification(entry.verification(), verificationElement.get_value().get_document());
		}
		ReadDownloadShards(entry.downloadShards(), dbDriveEntry["downloadShards"].get_array().value);
		ReadModificationShards(entry.dataModificationShards(), dbDriveEntry["dataModificationShards"].get_array().value);

		return entry;
	}

	// endregion
}}}
