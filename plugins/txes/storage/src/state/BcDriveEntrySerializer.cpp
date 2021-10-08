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
			io::Write16(output, modification.Folder.size());
			auto pFolder = (const uint8_t*) (modification.Folder.c_str());
			io::Write(output, utils::RawBuffer(pFolder, modification.Folder.size()));
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

		auto LoadActiveDataModification(io::InputStream& input) {
			Hash256 id;
			io::Read(input, id);
			Key owner;
			io::Read(input, owner);
			Hash256 downloadDataCdi;
			io::Read(input, downloadDataCdi);
			auto expectedUploadSize = io::Read64(input);
			auto actualUploadSize = io::Read64(input);
			auto folderSize = io::Read16(input);
			std::vector<uint8_t> folderBytes(folderSize);
			io::Read(input, folderBytes);
			std::string folder(folderBytes.begin(), folderBytes.end());
			return ActiveDataModification{ id, owner, downloadDataCdi, expectedUploadSize, actualUploadSize, folder };
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

		SaveActiveDataModifications(output, driveEntry.activeDataModifications());
		SaveCompletedDataModifications(output, driveEntry.completedDataModifications());
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

		LoadActiveDataModifications(input, entry.activeDataModifications());
		LoadCompletedDataModifications(input, entry.completedDataModifications());

		return entry;
	}
}}
