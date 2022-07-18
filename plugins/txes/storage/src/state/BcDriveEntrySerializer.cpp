/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "BcDriveEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	namespace {

		void SaveVerificationNode(io::OutputStream& output, const AVLTreeNode& node) {
			io::Write(output, node.Left);
			io::Write(output, node.Right);
			io::Write16(output, node.Height);
			io::Write32(output, node.Size);
		}

		void SaveActiveDataModification(io::OutputStream& output, const ActiveDataModification& modification) {
			io::Write(output, modification.Id);
			io::Write(output, modification.Owner);
			io::Write(output, modification.DownloadDataCdi);
			io::Write64(output, modification.ExpectedUploadSizeMegabytes);
			io::Write64(output, modification.ActualUploadSizeMegabytes);
			io::Write16(output, (uint16_t) modification.FolderName.size());
			io::Write8(output, modification.ReadyForApproval);
			auto pFolderName = (const uint8_t*) (modification.FolderName.c_str());
			io::Write(output, utils::RawBuffer(pFolderName, modification.FolderName.size()));
		}

		void SaveActiveDataModifications(io::OutputStream& output, const ActiveDataModifications& activeDataModifications) {
			io::Write16(output, utils::checked_cast<size_t, uint16_t>(activeDataModifications.size()));
			for (const auto& modification : activeDataModifications)
				SaveActiveDataModification(output, modification);
		}

		void SaveCompletedDataModifications(io::OutputStream& output, const CompletedDataModifications& completedDataModifications) {
			io::Write16(output, utils::checked_cast<size_t, uint16_t>(completedDataModifications.size()));
			for (const auto& modification : completedDataModifications) {
				SaveActiveDataModification(output,modification);
				io::Write8(output, utils::to_underlying_type(modification.State));
			}
		}

		template<typename TContainer>
		void SaveShard(io::OutputStream& output, const TContainer& shard) {
			io::Write8(output, utils::checked_cast<size_t, uint8_t>(shard.size()));
			for (const auto& key : shard) {
				io::Write(output, key);
			}
		}

		void SaveShards(io::OutputStream& output, const Shards& shards) {
			io::Write16(output, utils::checked_cast<size_t, uint16_t>(shards.size()));
			for (const auto& shard : shards) {
				SaveShard(output, shard);
			}
		}

		void SaveUploadInfo(io::OutputStream& output, const std::map<Key, uint64_t>& info) {
			io::Write16(output, info.size());
			for (const auto& [key, uploadValue]: info) {
				io::Write(output, key);
				io::Write64(output, uploadValue);
			}
		}

		void SaveModificationShardInfo(io::OutputStream& output, const ModificationShardInfo& shard) {
			SaveUploadInfo(output, shard.m_actualShardMembers);
			SaveUploadInfo(output, shard.m_formerShardMembers);
			io::Write64(output, shard.m_ownerUpload);
		}

		void SaveVerification(io::OutputStream& output, const std::optional<Verification>& verification) {
			bool hasVerification = verification.has_value();
			io::Write8(output, hasVerification);
			if (hasVerification) {
				io::Write(output, verification->VerificationTrigger);
				io::Write(output, verification->Expiration);
				io::Write32(output, verification->Duration);
				SaveShards(output, verification->Shards);
			}
		}

		void SaveConfirmedUsedSizes(io::OutputStream& output, const SizeMap& confirmedUsedSizes) {
			io::Write16(output, utils::checked_cast<size_t, uint16_t>(confirmedUsedSizes.size()));
			for (const auto& pair : confirmedUsedSizes) {
				io::Write(output, pair.first);
				io::Write64(output, pair.second);
			}
		}

		void SaveReplicators(io::OutputStream& output, const utils::SortedKeySet& replicators) {
			io::Write16(output, utils::checked_cast<size_t, uint16_t>(replicators.size()));
			for (const auto& replicatorKey : replicators)
				io::Write(output, replicatorKey);
		}

		void SaveDownloadShards(io::OutputStream& output, const DownloadShards& downloadShards) {
			io::Write16(output, utils::checked_cast<size_t, uint16_t>(downloadShards.size()));
			for (const auto& shard : downloadShards) {
				io::Write(output, shard);
			}
		}

		void SaveModificationShards(io::OutputStream& output, const ModificationShards& dataModificationShards) {
			io::Write16(output, utils::checked_cast<size_t, uint16_t>(dataModificationShards.size()));
			for (const auto& item : dataModificationShards) {
				auto& mainKey = item.first;
				io::Write(output, mainKey);
				SaveModificationShardInfo(output, item.second);
			}
		}

		void SaveConfirmedStorageInfos(io::OutputStream& output, const ConfirmedStorageInfos& infos) {
			io::Write16(output, infos.size());
			for (const auto& [key, info]: infos) {
				io::Write(output, key);
				io::Write(output, info.m_timeInConfirmedStorage);
				io::Write8(output, info.m_confirmedStorageSince.has_value());
				if (info.m_confirmedStorageSince) {
					io::Write(output, *info.m_confirmedStorageSince);
				}
			}
		}

		void LoadVerificationNode(io::InputStream& input, AVLTreeNode& node) {
			Key left;
			io::Read(input, left);
			Key right;
			io::Read(input, right);
			uint16_t height = io::Read16(input);
			uint32_t size = io::Read32(input);
			node = AVLTreeNode{left, right, height, size};
		}

		auto LoadActiveDataModification(io::InputStream& input) {
			Hash256 id;
			io::Read(input, id);
			Key owner;
			io::Read(input, owner);
			Hash256 downloadDataCdi;
			io::Read(input, downloadDataCdi);
			auto expectedUploadSize = io::Read64(input);
			auto actualUploadSize = io::Read64(input);
			auto folderNameSize = io::Read16(input);
			auto readyForApproval = io::Read8(input);
			std::vector<uint8_t> folderNameBytes(folderNameSize);
			io::Read(input, folderNameBytes);
			std::string folderName(folderNameBytes.begin(), folderNameBytes.end());
			return ActiveDataModification(id, owner, downloadDataCdi, expectedUploadSize, actualUploadSize, folderName, readyForApproval);
		}

		void LoadActiveDataModifications(io::InputStream& input, ActiveDataModifications& activeDataModifications) {
			auto count = io::Read16(input);
			while (count--) {
				auto modification = LoadActiveDataModification(input);
				activeDataModifications.emplace_back(modification);
			}
		}

		void LoadCompletedDataModifications(io::InputStream& input, CompletedDataModifications& completedDataModifications) {
			auto count = io::Read16(input);
			while (count--) {
				auto activeModification = LoadActiveDataModification(input);
				auto state = static_cast<DataModificationState>(io::Read8(input));
				completedDataModifications.emplace_back(activeModification, state);
			}
		}

		void LoadConfirmedUsedSizes(io::InputStream& input, SizeMap& confirmedUsedSizes) {
			auto count = io::Read16(input);
			while (count--) {
				Key replicatorKey;
				io::Read(input, replicatorKey);
				auto size = io::Read64(input);
				confirmedUsedSizes.emplace(replicatorKey, size);
			}
		}

		void LoadReplicators(io::InputStream& input, utils::SortedKeySet& replicators) {
			auto count = io::Read16(input);
			while (count--) {
				Key replicatorKey;
				io::Read(input, replicatorKey);
				replicators.insert(replicatorKey);
			}
		}

		void LoadShard(io::InputStream& input, std::vector<Key>& shard) {
			auto count = io::Read8(input);
			while (count--) {
				Key replicatorKey;
				io::Read(input, replicatorKey);
				shard.emplace_back(replicatorKey);
			}
		}

		void LoadShard(io::InputStream& input, std::set<Key>& shard) {
			auto count = io::Read8(input);
			while (count--) {
				Key replicatorKey;
				io::Read(input, replicatorKey);
				shard.emplace(replicatorKey);
			}
		}

		void LoadShards(io::InputStream& input, Shards& shards) {
			auto count = io::Read16(input);
			while (count--) {
				shards.emplace_back();
				LoadShard(input, shards.back());
			}
		}

		void LoadVerification(io::InputStream& input, std::optional<Verification>& verification) {
			bool hasVerification = io::Read8(input);
			if (hasVerification) {
				verification = Verification();
				io::Read(input, verification->VerificationTrigger);
				io::Read(input, verification->Expiration);
				verification->Duration = io::Read32(input);
				LoadShards(input, verification->Shards);
			}
		}

		void LoadDownloadShards(io::InputStream& input, DownloadShards& downloadShards) {
			auto count = io::Read16(input);
			while (count--) {
				Hash256 downloadChannelId;
				io::Read(input, downloadChannelId);
			}
		}

		void LoadUploadInfo(io::InputStream& input, std::map<Key, uint64_t>& info) {
			auto activeSize = io::Read16(input);
			while (activeSize--) {
				Key key;
				input.read(key);
				info[key] = io::Read64(input);
			}
		}

		void LoadModificationShardInfo(io::InputStream& input, ModificationShardInfo& info) {
			LoadUploadInfo(input, info.m_actualShardMembers);
			LoadUploadInfo(input, info.m_formerShardMembers);
			info.m_ownerUpload = io::Read64(input);
		}

		void LoadModificationShards(io::InputStream& input, ModificationShards& dataModificationShards) {
			auto count = io::Read16(input);
			while (count--) {
				Key mainKey;
				io::Read(input, mainKey);
				auto& shardInfo = dataModificationShards[mainKey];
				LoadModificationShardInfo(input, shardInfo);
			}
		}

		void LoadConfirmedStorageInfos(io::InputStream& input, ConfirmedStorageInfos& infos) {
			auto size = io::Read16(input);
			while (size--) {
				Key key;
				io::Read(input, key);

				ConfirmedStorageInfo info;
				io::Read(input, info.m_timeInConfirmedStorage);

				bool inConfirmed = io::Read8(input);

				if (inConfirmed) {
					Timestamp confirmedSince;
					io::Read(input, confirmedSince);
					info.m_confirmedStorageSince = confirmedSince;
				}

				infos[key] = info;
			}
		}
	}

	void BcDriveEntrySerializer::Save(const BcDriveEntry& driveEntry, io::OutputStream& output) {
		io::Write32(output, driveEntry.version());
		io::Write(output, driveEntry.key());

		io::Write(output, driveEntry.owner());
		io::Write(output, driveEntry.rootHash());
		io::Write64(output, driveEntry.size());
		io::Write64(output, driveEntry.usedSizeBytes());
		io::Write64(output, driveEntry.metaFilesSizeBytes());
		io::Write16(output, driveEntry.replicatorCount());

		io::Write(output, driveEntry.getQueuePrevious());
		io::Write(output, driveEntry.getQueueNext());
		io::Write(output, driveEntry.getLastPayment());

		SaveVerificationNode(output, driveEntry.verificationNode());
		SaveVerification(output, driveEntry.verification());

		SaveActiveDataModifications(output, driveEntry.activeDataModifications());
		SaveCompletedDataModifications(output, driveEntry.completedDataModifications());
		SaveConfirmedUsedSizes(output, driveEntry.confirmedUsedSizes());
		SaveReplicators(output, driveEntry.replicators());
		SaveReplicators(output, driveEntry.formerReplicators());
		SaveReplicators(output, driveEntry.offboardingReplicators());
		SaveDownloadShards(output, driveEntry.downloadShards());
		SaveModificationShards(output, driveEntry.dataModificationShards());
		SaveConfirmedStorageInfos(output, driveEntry.confirmedStorageInfos());
	}

	BcDriveEntry BcDriveEntrySerializer::Load(io::InputStream& input) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of DriveEntry", version);

		Key key;
		input.read(key);
		state::BcDriveEntry entry(key);
		entry.setVersion(version);

		Key owner;
		input.read(owner);
		entry.setOwner(owner);

		Hash256 rootHash;
		input.read(rootHash);
		entry.setRootHash(rootHash);

		entry.setSize(io::Read64(input));
		entry.setUsedSizeBytes(io::Read64(input));
		entry.setMetaFilesSizeBytes(io::Read64(input));
		entry.setReplicatorCount(io::Read16(input));

		Key queuePrevious;
		io::Read(input, queuePrevious);
		entry.setQueuePrevious(queuePrevious);

		Key queueNext;
		io::Read(input, queueNext);
		entry.setQueueNext(queueNext);

		Timestamp lastPayment;
		io::Read(input, lastPayment);
		entry.setLastPayment(lastPayment);

		LoadVerificationNode(input, entry.verificationNode());
		LoadVerification(input, entry.verification());

		LoadActiveDataModifications(input, entry.activeDataModifications());
		LoadCompletedDataModifications(input, entry.completedDataModifications());
		LoadConfirmedUsedSizes(input, entry.confirmedUsedSizes());
		LoadReplicators(input, entry.replicators());
		LoadReplicators(input, entry.formerReplicators());
		LoadReplicators(input, entry.offboardingReplicators());
		LoadDownloadShards(input, entry.downloadShards());
		LoadModificationShards(input, entry.dataModificationShards());
		LoadConfirmedStorageInfos(input, entry.confirmedStorageInfos());

		return entry;
	}
}}
