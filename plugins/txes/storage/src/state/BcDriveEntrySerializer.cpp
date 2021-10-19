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
		void SaveActiveDataModifications(io::OutputStream& output, const ActiveDataModifications& activeDataModifications) {
			io::Write16(output, utils::checked_cast<size_t, uint16_t>(activeDataModifications.size()));
			for (const auto& modification : activeDataModifications) {
				io::Write(output, modification.Id);
				io::Write(output, modification.Owner);
				io::Write(output, modification.DownloadDataCdi);
				io::Write64(output, modification.UploadSize);
			}
		}

		void SaveCompletedDataModifications(io::OutputStream& output, const CompletedDataModifications& completedDataModifications) {
			io::Write16(output, utils::checked_cast<size_t, uint16_t>(completedDataModifications.size()));
			for (const auto& modification : completedDataModifications) {
				io::Write(output, modification.Id);
				io::Write(output, modification.Owner);
				io::Write(output, modification.DownloadDataCdi);
				io::Write64(output, modification.UploadSize);
				io::Write8(output, utils::to_underlying_type(modification.State));
			}
		}

		void SaveConfirmedUsedSizes(io::OutputStream& output, const UsedSizeMap& confirmedUsedSizes) {
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

		void LoadActiveDataModifications(io::InputStream& input, ActiveDataModifications& activeDataModifications) {
			auto count = io::Read16(input);
			while (count--) {
				Hash256 id;
				io::Read(input, id);
				Key owner;
				io::Read(input, owner);
				Hash256 downloadDataCdi;
				io::Read(input, downloadDataCdi);
				auto uploadSize = io::Read64(input);
				activeDataModifications.emplace_back(ActiveDataModification{ id, owner, downloadDataCdi, uploadSize });
			}
		}

		void LoadCompletedDataModifications(io::InputStream& input, CompletedDataModifications& completedDataModifications) {
			auto count = io::Read16(input);
			while (count--) {
				Hash256 id;
				io::Read(input, id);
				Key owner;
				io::Read(input, owner);
				Hash256 downloadDataCdi;
				io::Read(input, downloadDataCdi);
				auto uploadSize = io::Read64(input);
				auto state = static_cast<DataModificationState>(io::Read8(input));
				completedDataModifications.emplace_back(ActiveDataModification{ id, owner, downloadDataCdi, uploadSize }, state);
			}
		}

		void LoadConfirmedUsedSizes(io::InputStream& input, UsedSizeMap& confirmedUsedSizes) {
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
		SaveConfirmedUsedSizes(output, driveEntry.confirmedUsedSizes());
		SaveReplicators(output, driveEntry.replicators());
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
		LoadConfirmedUsedSizes(input, entry.confirmedUsedSizes());
		LoadReplicators(input, entry.replicators());

		return entry;
	}
}}
