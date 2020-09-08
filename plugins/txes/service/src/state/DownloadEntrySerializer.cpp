/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DownloadEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"

namespace catapult { namespace state {

	void DownloadEntrySerializer::Save(const DownloadEntry& entry, io::OutputStream& output) {
		// write version
		io::Write32(output, 1);

		io::Write(output, entry.OperationToken);
		io::Write(output, entry.DriveKey);
		io::Write(output, entry.FileRecipient);
		io::Write(output, entry.Height);
		io::Write16(output, utils::checked_cast<size_t, uint16_t>(entry.Files.size()));
		for (const auto& pair : entry.Files) {
			io::Write(output, pair.first);
			io::Write64(output, pair.second);
		}
	}

	DownloadEntry DownloadEntrySerializer::Load(io::InputStream& input) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of DownloadEntry", version);

		DownloadEntry entry;
		io::Read(input, entry.OperationToken);
		io::Read(input, entry.DriveKey);
		io::Read(input, entry.FileRecipient);
		io::Read(input, entry.Height);
		auto fileCount = io::Read16(input);
		while (fileCount--) {
			Hash256 fileHash;
			io::Read(input, fileHash);
			auto fileSize = io::Read64(input);
			entry.Files.emplace(fileHash, fileSize);
		}

		return entry;
	}
}}
