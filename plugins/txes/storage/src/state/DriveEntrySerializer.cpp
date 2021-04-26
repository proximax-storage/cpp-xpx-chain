/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DriveEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	void DriveEntrySerializer::Save(const DriveEntry& driveEntry, io::OutputStream& output) {

		io::Write32(output, driveEntry.version());
		io::Write(output, driveEntry.key());

		io::Write(output, driveEntry.owner());
		io::Write64(output, driveEntry.size());
		io::Write16(output, driveEntry.replicatorCount());
	}

	DriveEntry DriveEntrySerializer::Load(io::InputStream& input) {

		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of DriveEntry", version);

		Key key;
		input.read(key);
		state::DriveEntry entry(key);
		entry.setVersion(version);

		Key owner;
		input.read(owner);
		entry.setOwner(owner);

		entry.setSize(io::Read64(input));
		entry.setReplicatorCount(io::Read16(input));

		return entry;
	}
}}
