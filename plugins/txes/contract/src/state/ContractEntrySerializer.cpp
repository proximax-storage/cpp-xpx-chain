/**
*** Copyright (c) 2018-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "ContractEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"

namespace catapult { namespace state {

	namespace {
		void SaveKeySet(io::OutputStream& output, const utils::SortedKeySet& keySet) {
			io::Write64(output, keySet.size());
			for (const auto& key : keySet)
				io::Write(output, key);
		}
	}

	void ContractEntrySerializer::Save(const ContractEntry& entry, io::OutputStream& output) {
		io::Write64(output, entry.start().unwrap());
		io::Write64(output, entry.duration().unwrap());
		io::Write(output, entry.hash());
		io::Write(output, entry.key());

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
	}

	ContractEntry ContractEntrySerializer::Load(io::InputStream& input) {
		auto start = Height{io::Read64(input)};
		auto duration = BlockDuration{io::Read64(input)};
		Hash256 hash;
		input.read(hash);
		Key key;
		input.read(key);

		auto entry = state::ContractEntry(key);
		entry.setStart(start);
		entry.setDuration(duration);
		entry.setHash(hash);

		LoadKeySet(input, entry.customers());
		LoadKeySet(input, entry.executors());
		LoadKeySet(input, entry.verifiers());

		return entry;
	}
}}
