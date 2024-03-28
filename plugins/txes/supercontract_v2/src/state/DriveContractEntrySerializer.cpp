/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DriveContractEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/exceptions.h"

namespace catapult { namespace state {

    void DriveContractEntrySerializer::Save(const DriveContractEntry& entry, io::OutputStream& output) {
        // write version
		io::Write32(output, 1);

        io::Write(output, entry.key());
        io::Write(output, entry.contractKey());
    }

    DriveContractEntry DriveContractEntrySerializer::Load(io::InputStream& input) {
        // read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of DriveContractEntry", version);

        Key key;
		input.read(key);
		state::DriveContractEntry entry(key);

        Key contractKey;
		input.read(contractKey);
        entry.setContractKey(contractKey);

		return entry;
    }
}}