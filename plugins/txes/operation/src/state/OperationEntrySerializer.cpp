/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "OperationEntrySerializer.h"

namespace catapult { namespace state {

	void OperationEntryExtendedDataSerializer::Save(const OperationEntry& entry, io::OutputStream& output) {
		// write version
		io::Write32(output, 1);

		output.write(entry.OperationToken);
		io::Write16(output, utils::checked_cast<size_t, uint16_t>(entry.Executors.size()));
		for (const auto& executor : entry.Executors)
			io::Write(output, executor);
	}

	void OperationEntryExtendedDataSerializer::Load(io::InputStream& input, OperationEntry& entry) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of OperationEntry", version);

		input.read(entry.OperationToken);
		auto executorCount = io::Read16(input);
		while (executorCount--) {
			Key executor;
			io::Read(input, executor);
			entry.Executors.insert(executor);
		}
	}
}}
