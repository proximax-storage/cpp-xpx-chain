/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DriveEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/exceptions.h"

namespace catapult { namespace state {

	namespace {
		void SaveContractorDepositMap(io::OutputStream& output, const ContractorDepositMap& contractorMap) {
			io::Write64(output, contractorMap.size());
			for (const auto& contractor : contractorMap) {
				io::Write(output, contractor.first);
				io::Write64(output, contractor.second.size());
				for (const auto& pair : contractor.second) {
					io::Write(output, pair.first);
					io::Write(output, pair.second.MosaicId);
					io::Write(output, pair.second.Amount);
				}
			}
		}
	}

	void DriveEntrySerializer::Save(const DriveEntry& entry, io::OutputStream& output) {
		// write version
		io::Write32(output, 1);

		io::Write64(output, entry.start().unwrap());
		io::Write64(output, entry.duration().unwrap());
		io::Write64(output, entry.size());
		io::Write16(output, entry.replicas());
		io::Write(output, entry.key());

		SaveContractorDepositMap(output, entry.customers());
		SaveContractorDepositMap(output, entry.replicators());
	}

	namespace {
		void LoadContractorDepositMap(io::InputStream& input, ContractorDepositMap& contractorDepositMap) {
			auto numKeys = io::Read64(input);
			while (numKeys--) {
				Key key;
				input.read(key);
				auto numHashes = io::Read64(input);
				DepositMap depositMap;
				while (numHashes--) {
					Hash256 fileHash;
					input.read(fileHash);
					UnresolvedMosaicId mosaicId;
					io::Read(input, mosaicId);
					Amount amount;
					io::Read(input, amount);
					depositMap.emplace(fileHash, model::UnresolvedMosaic{mosaicId, amount});
				}
				contractorDepositMap.emplace(key, depositMap);
			}
		}
	}

	DriveEntry DriveEntrySerializer::Load(io::InputStream& input) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of DriveEntry", version);

		auto start = Height{io::Read64(input)};
		auto duration = BlockDuration{io::Read64(input)};
		auto size = io::Read64(input);
		auto replicas = io::Read16(input);
		Key key;
		input.read(key);

		auto entry = state::DriveEntry(key);
		entry.setStart(start);
		entry.setDuration(duration);
		entry.setSize(size);
		entry.setReplicas(replicas);

		LoadContractorDepositMap(input, entry.customers());
		LoadContractorDepositMap(input, entry.replicators());

		return entry;
	}
}}
