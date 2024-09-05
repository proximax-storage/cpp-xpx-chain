/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DbrbProcessEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	void DbrbProcessEntrySerializer::Save(const DbrbProcessEntry& entry, io::OutputStream& output) {
		io::Write32(output, entry.version());
		io::Write(output, entry.processId());
		io::Write(output, entry.expirationTime());

		if (entry.version() > 1)
			io::Write(output, entry.banPeriod());
	}

	DbrbProcessEntry DbrbProcessEntrySerializer::Load(io::InputStream& input) {
		VersionType version = io::Read32(input);
		if (version > 2)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of DbrbProcessEntry", version);

		dbrb::ProcessId processId;
		io::Read(input, processId);

		Timestamp expirationTime;
		io::Read(input, expirationTime);

		BlockDuration banPeriod(0);
		if (version > 1)
			io::Read(input, banPeriod);

		return state::DbrbProcessEntry(processId, expirationTime, version, banPeriod);
	}
}}
