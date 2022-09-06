/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "GlobalEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	void GlobalEntrySerializer::Save(const GlobalEntry& entry, io::OutputStream& output) {
		io::Write32(output, 1);
		io::Write(output, entry.GetKey());
		auto data = entry.Get();
		io::Write32(output, data.size());
		io::Write(output, data);
	}

	GlobalEntry GlobalEntrySerializer::Load(io::InputStream& input) {
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of GlobalEntry", version);
		Hash256 Key;

		io::Read(input, Key);
		std::vector<uint8_t> data;
		auto entrySize = io::Read32(input);
		data.resize(entrySize);
		input.read({data.data(), entrySize});
		return GlobalEntry(std::move(Key), std::move(data));
	}
}}
