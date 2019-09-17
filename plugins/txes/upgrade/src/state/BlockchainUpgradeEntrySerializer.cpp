/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "BlockchainUpgradeEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/exceptions.h"

namespace catapult { namespace state {

	void BlockchainUpgradeEntrySerializer::Save(const BlockchainUpgradeEntry& entry, io::OutputStream& output) {
		// write version
		io::Write32(output, 1);

		io::Write64(output, entry.height().unwrap());
		io::Write64(output, entry.blockChainVersion().unwrap());
	}

	BlockchainUpgradeEntry BlockchainUpgradeEntrySerializer::Load(io::InputStream& input) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of BlockchainUpgradeEntry", version);

		auto height = Height{io::Read64(input)};
		auto blockChainVersion = BlockchainVersion{io::Read64(input)};

		return state::BlockchainUpgradeEntry(height, blockChainVersion);
	}
}}
