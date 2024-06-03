/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "BootKeyReplicatorEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	void BootKeyReplicatorEntrySerializer::Save(const BootKeyReplicatorEntry& replicatorEntry, io::OutputStream& output) {

		io::Write32(output, replicatorEntry.version());
		io::Write(output, replicatorEntry.nodeBootKey());
		io::Write(output, replicatorEntry.replicatorKey());
	}

	BootKeyReplicatorEntry BootKeyReplicatorEntrySerializer::Load(io::InputStream& input) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of BootKeyReplicatorEntry", version);

		Key nodeBootKey;
		input.read(nodeBootKey);
		Key replicatorKey;
		input.read(replicatorKey);
		state::BootKeyReplicatorEntry entry(nodeBootKey, replicatorKey);
		entry.setVersion(version);

		return entry;
	}
}}
