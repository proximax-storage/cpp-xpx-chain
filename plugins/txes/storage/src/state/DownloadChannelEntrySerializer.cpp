/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DownloadChannelEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	namespace {

		void SaveListOfPublicKeys(io::OutputStream& output, const std::vector<Key>& listOfPublicKeys) {
			io::Write16(output, listOfPublicKeys.size());
			for (const auto& key : listOfPublicKeys)
				io::Write(output, key);
		}

		void SaveCumulativePayments(io::OutputStream& output, const std::map<Key, Amount>& cumulativePayments) {
			io::Write16(output, cumulativePayments.size());
			for (const auto& pair : cumulativePayments) {
				io::Write(output, pair.first);
				io::Write(output, pair.second);
			}
		}

		void LoadListOfPublicKeys(io::InputStream& input, std::vector<Key>& listOfPublicKeys) {
			auto keyCount = io::Read16(input);
			while (keyCount--) {
				Key key;
				io::Read(input, key);
				listOfPublicKeys.push_back(key);
			}
		}

		void LoadCumulativePayments(io::InputStream& input, std::map<Key, Amount>& cumulativePayments) {
			auto pairCount = io::Read16(input);
			while (pairCount--) {
				Key replicator;
				io::Read(input, replicator);
				cumulativePayments[replicator] = Amount(io::Read64(input));
			}
		}

	}

	void DownloadChannelEntrySerializer::Save(const DownloadChannelEntry& downloadEntry, io::OutputStream& output) {
		auto version = downloadEntry.version();

		io::Write32(output, version);

		io::Write(output, downloadEntry.id());
		io::Write(output, downloadEntry.consumer());
		io::Write(output, downloadEntry.drive());
		io::Write64(output, downloadEntry.downloadSize());
		io::Write16(output, downloadEntry.downloadApprovalCount());

		SaveListOfPublicKeys(output, downloadEntry.listOfPublicKeys());
		SaveCumulativePayments(output, downloadEntry.cumulativePayments());
	}

	DownloadChannelEntry DownloadChannelEntrySerializer::Load(io::InputStream& input) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of DownloadChannelEntry", version);

		Hash256 id;
		input.read(id);
		state::DownloadChannelEntry entry(id);
		entry.setVersion(version);

		Key consumer;
		input.read(consumer);
		entry.setConsumer(consumer);

		Key drive;
		input.read(drive);
		entry.setDrive(drive);

		entry.setDownloadSize(io::Read64(input));
		entry.setDownloadApprovalCount(io::Read16(input));

		LoadListOfPublicKeys(input, entry.listOfPublicKeys());
		LoadCumulativePayments(input, entry.cumulativePayments());

		return entry;
	}
}}
