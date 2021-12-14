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
		void SaveActiveDataModification(io::OutputStream& output, const ActiveDataModification& modification) {
			io::Write(output, modification.Id);
			io::Write(output, modification.Owner);
			io::Write(output, modification.DownloadDataCdi);
			io::Write64(output, modification.ExpectedUploadSize);
			io::Write64(output, modification.ActualUploadSize);
			io::Write16(output, (uint16_t) modification.FolderName.size());
			io::Write8(output, modification.ReadyForApproval);
			auto pFolderName = (const uint8_t*) (modification.FolderName.c_str());
			io::Write(output, utils::RawBuffer(pFolderName, modification.FolderName.size()));
		}

		void SaveActiveDataModifications(io::OutputStream& output, const ActiveDataModifications& activeDataModifications) {
			io::Write16(output, utils::checked_cast<size_t, uint16_t>(activeDataModifications.size()));
			for (const auto& modification : activeDataModifications) {
				SaveActiveDataModification(output, modification);
			}
		}

		void SaveCompletedDataModifications(io::OutputStream& output, const CompletedDataModifications& completedDataModifications) {
			io::Write16(output, utils::checked_cast<size_t, uint16_t>(completedDataModifications.size()));
			for (const auto& modification : completedDataModifications) {
				SaveActiveDataModification(output,modification);
				io::Write8(output, utils::to_underlying_type(modification.State));
			}
		}

		void SaveVerificationOpinions(io::OutputStream& output, const VerificationResults& opinions) {
			io::Write16(output, opinions.size());
			for (const auto& pair : opinions) {
				io::Write(output, pair.first);
				io::Write8(output, pair.second);
			}
		}

		void SaveVerifications(io::OutputStream& output, const Verifications& verifications) {
			io::Write16(output, utils::checked_cast<size_t, uint16_t>(verifications.size()));
			for (const auto& verification : verifications) {
				io::Write(output, verification.VerificationTrigger);
				io::Write8(output, utils::to_underlying_type(verification.State));
				SaveVerificationOpinions(output, verification.Results);
			}
		}

		void SaveConfirmedUsedSizes(io::OutputStream& output, const SizeMap& confirmedUsedSizes) {
			io::Write16(output, utils::checked_cast<size_t, uint16_t>(confirmedUsedSizes.size()));
			for (const auto& pair : confirmedUsedSizes) {
				io::Write(output, pair.first);
				io::Write64(output, pair.second);
			}
		}

		void SaveReplicators(io::OutputStream& output, const utils::KeySet& replicators) {
			io::Write16(output, utils::checked_cast<size_t, uint16_t>(replicators.size()));
			for (const auto& replicatorKey : replicators)
				io::Write(output, replicatorKey);
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

		void LoadReplicators(io::InputStream& input, utils::KeySet& replicators) {
			auto count = io::Read16(input);
			while (count--) {
				Key replicatorKey;
				io::Read(input, replicatorKey);
				replicators.insert(replicatorKey);
			}
		}

		void LoadVerificationOpinions(io::InputStream& input, VerificationResults& opinions) {
			auto pairCount = io::Read16(input);
			while (pairCount--) {
				Key prover;
				io::Read(input, prover);
				opinions[prover] = io::Read8(input);
			}
		}

		void LoadVerifications(io::InputStream& input, Verifications& verifications) {
		    auto count = io::Read16(input);
		    while (count--) {
		        state::Verification verification;
		        io::Read(input, verification.VerificationTrigger);
				verification.State = static_cast<VerificationState>(io::Read8(input));
				LoadVerificationOpinions(input, verification.Results);

		        verifications.emplace_back(verification);
		    }
		}
	}

	void BcDriveEntrySerializer::Save(const BcDriveEntry& driveEntry, io::OutputStream& output) {

		io::Write32(output, driveEntry.version());
		io::Write(output, driveEntry.key());

		io::Write(output, driveEntry.owner());
		io::Write(output, driveEntry.rootHash());
		io::Write64(output, driveEntry.size());
		io::Write64(output, driveEntry.usedSize());
		io::Write64(output, driveEntry.metaFilesSize());
		io::Write16(output, driveEntry.replicatorCount());
		io::Write64(output, driveEntry.ownerCumulativeUploadSize());

		SaveActiveDataModifications(output, driveEntry.activeDataModifications());
		SaveCompletedDataModifications(output, driveEntry.completedDataModifications());
		SaveConfirmedUsedSizes(output, driveEntry.confirmedUsedSizes());
		SaveReplicators(output, driveEntry.replicators());
		SaveVerifications(output, driveEntry.verifications());
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
		entry.setUsedSize(io::Read64(input));
		entry.setMetaFilesSize(io::Read64(input));
		entry.setReplicatorCount(io::Read16(input));
		entry.setOwnerCumulativeUploadSize(io::Read64(input));

		LoadActiveDataModifications(input, entry.activeDataModifications());
		LoadCompletedDataModifications(input, entry.completedDataModifications());
		LoadConfirmedUsedSizes(input, entry.confirmedUsedSizes());
		LoadReplicators(input, entry.replicators());
		LoadVerifications(input, entry.verifications());

		return entry;
	}
}}
