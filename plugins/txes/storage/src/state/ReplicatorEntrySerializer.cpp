/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ReplicatorEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	namespace {

		void SaveDrives(io::OutputStream& output, const DrivesMap& drives) {
			io::Write16(output, utils::checked_cast<size_t, uint16_t>(drives.size()));
			for (const auto& drivePair : drives) {
				io::Write(output, drivePair.first);
				io::Write(output, drivePair.second.LastApprovedDataModificationId);
				io::Write8(output, drivePair.second.DataModificationIdIsValid);
				io::Write64(output, drivePair.second.InitialDownloadWork);
			}
		}

		void LoadDrives(io::InputStream& input, DrivesMap& drives) {
			auto count = io::Read16(input);
			while (count--) {
				Key drive;
				io::Read(input, drive);

				DriveInfo info;
				io::Read(input, info.LastApprovedDataModificationId);
				info.DataModificationIdIsValid = io::Read8(input);
				info.InitialDownloadWork = io::Read64(input);

				drives.emplace(drive, info);
			}
		}

	}

	void ReplicatorEntrySerializer::Save(const ReplicatorEntry& replicatorEntry, io::OutputStream& output) {

		io::Write32(output, replicatorEntry.version());
		io::Write(output, replicatorEntry.key());

		io::Write(output, replicatorEntry.capacity());
		io::Write16(output, utils::checked_cast<size_t, uint16_t>(replicatorEntry.drives().size()));

		SaveDrives(output, replicatorEntry.drives());
	}

	ReplicatorEntry ReplicatorEntrySerializer::Load(io::InputStream& input) {

		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of ReplicatorEntry", version);

		Key key;
		input.read(key);
		state::ReplicatorEntry entry(key);
		entry.setVersion(version);

		entry.setCapacity(Amount(io::Read64(input)));

		LoadDrives(input, entry.drives());

		return entry;
	}
}}
