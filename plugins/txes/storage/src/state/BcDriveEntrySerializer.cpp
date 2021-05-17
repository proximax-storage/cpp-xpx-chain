/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "BcDriveEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	namespace {

		void SaveActiveDataModifications(io::OutputStream& output, const std::vector<Hash256>& activeDataModifications) {
			io::Write64(output, activeDataModifications.size());
			for (const auto& hash : activeDataModifications) {
				io::Write(output, hash);
			}
		}

		void SaveCompletedDataModifications(io::OutputStream& output, const std::vector<std::pair<Hash256, DataModificationState>>& completedDataModifications) {
			io::Write64(output, completedDataModifications.size());
			for (const auto& pair : completedDataModifications) {
				io::Write(output, pair.first);
				io::Write8(output, utils::to_underlying_type(pair.second));
			}
		}

		void LoadActiveDataModifications(io::InputStream& input, std::vector<Hash256>& activeDataModifications) {
			auto hashCount = io::Read64(input);
			while (hashCount--) {
				Hash256 hash;
				io::Read(input, hash);
				activeDataModifications.push_back(hash);
			}
		}

		void LoadCompletedDataModifications(io::InputStream& input, std::vector<std::pair<Hash256, DataModificationState>>& completedDataModifications) {
			auto pairCount = io::Read64(input);
			while (pairCount--) {
				Hash256 hash;
				io::Read(input, hash);
				auto state = static_cast<DataModificationState>(io::Read8(input));
				completedDataModifications.emplace_back(hash, state);
			}
		}

	}

	void BcDriveEntrySerializer::Save(const BcDriveEntry& driveEntry, io::OutputStream& output) {

		io::Write32(output, driveEntry.version());
		io::Write(output, driveEntry.key());

		io::Write(output, driveEntry.owner());
		io::Write(output, driveEntry.rootHash());
		io::Write64(output, driveEntry.size());
		io::Write16(output, driveEntry.replicatorCount());

		SaveActiveDataModifications(output, driveEntry.activeDataModifications());
		SaveCompletedDataModifications(output, driveEntry.completedDataModifications());
	}

	BcDriveEntry BcDriveEntrySerializer::Load(io::InputStream& input) {

		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of DriveEntry", version);

		Key key;
		input.read(key);
		state::BcDriveEntry entry(key);
		entry.setVersion(version);

		Key owner;
		input.read(owner);
		entry.setOwner(owner);

		Hash256 rootHash;
		input.read(rootHash);
		entry.setRootHash(rootHash);

		entry.setSize(io::Read64(input));
		entry.setReplicatorCount(io::Read16(input));

		LoadActiveDataModifications(input, entry.activeDataModifications());
		LoadCompletedDataModifications(input, entry.completedDataModifications());

		return entry;
	}
}}
