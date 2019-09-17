/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "FileEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/exceptions.h"

namespace catapult { namespace state {

	void FileEntrySerializer::Save(const FileEntry& entry, io::OutputStream& output) {
		// write version
		io::Write32(output, 1);

		io::Write(output, entry.key());
		io::Write(output, entry.parentKey());

		const auto& fileName = entry.name();
		io::Write8(output, fileName.size());
		io::Write(output, RawBuffer((const uint8_t*)fileName.data(), fileName.size()));
	}

	FileEntry FileEntrySerializer::Load(io::InputStream& input) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of FileEntry", version);

		DriveFileKey key;
		input.read(key);

		DriveFileKey parentKey;
		input.read(parentKey);

		std::string fileName;
		uint8_t fileNameSize = io::Read8(input);
		fileName.resize(fileNameSize);
		io::Read(input, MutableRawBuffer((uint8_t*)fileName.data(), fileName.size()));

		auto entry = state::FileEntry(key);
		entry.setParentKey(parentKey);
		entry.setName(fileName);

		return entry;
	}
}}
