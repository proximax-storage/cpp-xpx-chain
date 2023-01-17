/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MessageHashEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	void MessageHashEntrySerializer::Save(const MessageHashEntry& entry, io::OutputStream& output) {
		io::Write32(output, entry.version());
		io::Write8(output, entry.key());
		io::Write(output, entry.hash());
	}

	MessageHashEntry MessageHashEntrySerializer::Load(io::InputStream& input) {
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of MessageHashEntry", version);

		const uint8_t key = io::Read8(input);
		state::MessageHashEntry entry(key);

		Hash256 hash;
		io::Read(input, hash);
		entry.setHash(hash);

		return entry;
	}
}}
