/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ReplicatorEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"

namespace catapult { namespace state {

	void  ReplicatorEntrySerializer::Save(const ReplicatorEntry& replicatorEntry, io::OutputStream& output) {

		io::Write32(output, replicatorEntry.version());
		io::Write(output, replicatorEntry.key());

		io::Write(output, replicatorEntry.capacity());
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

		return entry;
	}
}}
