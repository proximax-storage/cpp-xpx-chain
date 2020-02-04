/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "OperationEntrySerializer.h"
#include "src/catapult/functions.h"

namespace catapult { namespace state {

	namespace {
		template<typename TArray>
		void SaveArray(const TArray& array, io::OutputStream& output) {
			io::Write16(output, utils::checked_cast<size_t, uint16_t>(array.size()));
			for (const auto& item : array)
				io::Write(output, item);
		}

		template<typename TValue>
		void LoadArray(io::InputStream& input, consumer<const TValue&> inserter) {
			auto count = io::Read16(input);
			while (count--) {
				TValue item;
				io::Read(input, item);
				inserter(item);
			}
		}
	}

	void OperationEntryExtendedDataSerializer::Save(const OperationEntry& entry, io::OutputStream& output) {
		// write version
		io::Write32(output, 1);

		output.write(entry.OperationToken);
		SaveArray(entry.Executors, output);
		SaveArray(entry.TransactionHashes, output);
	}

	void OperationEntryExtendedDataSerializer::Load(io::InputStream& input, OperationEntry& entry) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of OperationEntry", version);

		input.read(entry.OperationToken);
		LoadArray<Key>(input, [&executors = entry.Executors](const auto& executor) { executors.insert(executor); });
		LoadArray<Hash256>(input, [&hashes = entry.TransactionHashes](const auto& hash) { hashes.push_back(hash); });
	}
}}
