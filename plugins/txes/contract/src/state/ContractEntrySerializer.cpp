/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ContractEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/exceptions.h"

namespace catapult { namespace state {

	namespace {
		void SaveKeySet(io::OutputStream& output, const utils::SortedKeySet& keySet) {
			io::Write64(output, keySet.size());
			for (const auto& key : keySet)
				io::Write(output, key);
		}

		void SaveHashes(io::OutputStream& output, const std::vector<model::HashSnapshot>& hashes) {
			io::Write64(output, hashes.size());
			for (const auto& hashSnpashot : hashes) {
				io::Write(output, hashSnpashot.Hash);
				io::Write64(output, hashSnpashot.HashHeight.unwrap());
			}
		}
	}

	void ContractEntrySerializer::Save(const ContractEntry& entry, io::OutputStream& output) {
		// write version
		io::Write32(output, 1);

		io::Write64(output, entry.start().unwrap());
		io::Write64(output, entry.duration().unwrap());
		io::Write(output, entry.key());

		SaveHashes(output, entry.hashes());
		SaveKeySet(output, entry.customers());
		SaveKeySet(output, entry.executors());
		SaveKeySet(output, entry.verifiers());
	}

	namespace {
		void LoadKeySet(io::InputStream& input, utils::SortedKeySet& keySet) {
			auto numKeys = io::Read64(input);
			while (numKeys--) {
				Key key;
				input.read(key);
				keySet.insert(key);
			}
		}

		void LoadHashes(io::InputStream& input, state::ContractEntry& entry) {
			auto numKeys = io::Read64(input);
			while (numKeys--) {
				Hash256 hash;
				input.read(hash);
				entry.pushHash(hash, Height(io::Read64(input)));
			}
		}
	}

	ContractEntry ContractEntrySerializer::Load(io::InputStream& input) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of ContractEntry", version);

		auto start = Height{io::Read64(input)};
		auto duration = BlockDuration{io::Read64(input)};
		Key key;
		input.read(key);

		auto entry = state::ContractEntry(key);
		entry.setStart(start);
		entry.setDuration(duration);

		LoadHashes(input, entry);
		LoadKeySet(input, entry.customers());
		LoadKeySet(input, entry.executors());
		LoadKeySet(input, entry.verifiers());

		return entry;
	}
}}
