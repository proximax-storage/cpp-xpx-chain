/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "BlsKeysEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	void BlsKeysEntrySerializer::Save(const BlsKeysEntry& blsKeysEntry, io::OutputStream& output) {

		io::Write32(output, blsKeysEntry.version());
		io::Write(output, blsKeysEntry.blsKey());

		io::Write(output, blsKeysEntry.key());
	}

	BlsKeysEntry BlsKeysEntrySerializer::Load(io::InputStream& input) {

		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of BlsKeysEntry", version);

		BLSPublicKey blsKey;
		input.read(blsKey);
		state::BlsKeysEntry entry(blsKey);
		entry.setVersion(version);

		Key key;
		input.read(key);
		entry.setKey(key);

		return entry;
	}
}}
