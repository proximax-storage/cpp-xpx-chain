/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommitteeEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/exceptions.h"

namespace catapult { namespace state {

	void CommitteeEntrySerializer::Save(const CommitteeEntry& entry, io::OutputStream& output) {
		// write version
		io::Write32(output, entry.version());

		io::Write(output, entry.key());
		io::Write(output, entry.owner());
		io::Write64(output, entry.disabledHeight().unwrap());
		io::Write64(output, entry.lastSigningBlockHeight().unwrap());
		io::Write64(output, entry.effectiveBalance().unwrap());
		io::Write8(output, entry.canHarvest());
		io::WriteDouble(output, entry.activity());
		io::WriteDouble(output, entry.greed());

		if (entry.version() > 1)
			io::Write64(output, entry.expirationTime().unwrap());
	}

	CommitteeEntry CommitteeEntrySerializer::Load(io::InputStream& input) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 2)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of CommitteeEntry", version);

		Key key;
		input.read(key);
		Key owner;
		input.read(owner);
		auto disabledHeight = Height{io::Read64(input)};
		auto lastSigningBlockHeight = Height{io::Read64(input)};
		auto effectiveBalance = Importance{io::Read64(input)};
		auto canHarvest = io::Read8(input);
		auto activity = io::ReadDouble(input);
		auto greed = io::ReadDouble(input);

		auto expirationTime = Timestamp(0);
		if (version > 1)
			expirationTime = Timestamp{io::Read64(input)};

		return state::CommitteeEntry(key, owner, lastSigningBlockHeight, effectiveBalance, canHarvest, activity, greed, disabledHeight, version, expirationTime);
	}
}}
