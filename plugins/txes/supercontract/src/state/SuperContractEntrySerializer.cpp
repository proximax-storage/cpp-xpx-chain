/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SuperContractEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/exceptions.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	void SuperContractEntrySerializer::Save(const SuperContractEntry& entry, io::OutputStream& output) {
		// write version
		io::Write32(output, 1);

		io::Write(output, entry.key());
		io::Write(output, entry.start());
		io::Write(output, entry.end());
		io::Write(output, entry.mainDriveKey());
		io::Write(output, entry.fileHash());
		io::Write(output, entry.vmVersion());
	}

	SuperContractEntry SuperContractEntrySerializer::Load(io::InputStream& input) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of SuperContractEntry", version);

		Key key;
		input.read(key);
		state::SuperContractEntry entry(key);

		entry.setStart(Height(io::Read64(input)));
		entry.setEnd(Height(io::Read64(input)));

		Key mainDriveKey;
		input.read(mainDriveKey);
		entry.setMainDriveKey(mainDriveKey);

		Hash256 fileHash;
		input.read(fileHash);
		entry.setFileHash(fileHash);

		entry.setVmVersion(VmVersion(io::Read64(input)));

		return entry;
	}
}}
