/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "QueueEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	void QueueEntrySerializer::Save(const QueueEntry& entry, io::OutputStream& output) {
		io::Write32(output, entry.version());
		io::Write(output, entry.getFirst());
		io::Write(output, entry.getLast());
	}

	QueueEntry QueueEntrySerializer::Load(io::InputStream& input) {

		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of DriveEntry", version);

		Key key;
		input.read(key);
		state::QueueEntry entry(key);

		Key first;
		input.read(first);
		entry.setFirst(first);

		Key last;
		input.read(last);
		entry.setLast(last);

		return entry;
	}
}}
