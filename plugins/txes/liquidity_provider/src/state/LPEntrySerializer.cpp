/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "LPEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	namespace {

	}

	void LiquidityProviderEntrySerializer::Save(const LiquidityProviderEntry& entry, io::OutputStream& output) {

		io::Write32(output, entry.version());
		io::Write64(output, entry.mosaicId().unwrap());

		io::Write(output, entry.owner());
		io::Write64(output, entry.initiallyMinted().unwrap());
		io::Write64(output, entry.additionallyMinted().unwrap());
		io::Write(output, entry.slashingAccount());
	}

	LiquidityProviderEntry LiquidityProviderEntrySerializer::Load(io::InputStream& input) {

		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of DriveEntry", version);

		MosaicId mosaicId = MosaicId{io::Read64(input)};
		state::LiquidityProviderEntry entry(mosaicId);
		entry.setVersion(version);

		Key owner;
		input.read(owner);
		entry.setOwner(owner);

		entry.setInitiallyMinted(Amount{io::Read64(input)});
		entry.setAdditionallyMinted(Amount{io::Read64(input)});

		Key slashingAccount;
		io::Read(input, slashingAccount);
		entry.setSlashingAccount(slashingAccount);

		return entry;
	}
}}
