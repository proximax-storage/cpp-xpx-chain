/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CatapultUpgradeEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/exceptions.h"

namespace catapult { namespace state {

	void CatapultUpgradeEntrySerializer::Save(const CatapultUpgradeEntry& entry, io::OutputStream& output) {
		// write version
		io::Write32(output, 1);

		io::Write64(output, entry.height().unwrap());
		io::Write64(output, entry.catapultVersion().unwrap());
	}

	CatapultUpgradeEntry CatapultUpgradeEntrySerializer::Load(io::InputStream& input) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of CatapultUpgradeEntry", version);

		auto height = Height{io::Read64(input)};
		auto catapultVersion = CatapultVersion{io::Read64(input)};

		return state::CatapultUpgradeEntry(height, catapultVersion);
	}
}}
