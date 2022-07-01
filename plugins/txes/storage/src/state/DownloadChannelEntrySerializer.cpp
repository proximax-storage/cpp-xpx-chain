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

		void SaveShardReplicators(io::OutputStream& output, const utils::SortedKeySet& shardReplicators) {
			io::Write16(output, utils::checked_cast<size_t, uint16_t>(shardReplicators.size()));
			for (const auto& replicatorKey : shardReplicators)
				io::Write(output, replicatorKey);
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

		void LoadShardReplicators(io::InputStream& input, utils::SortedKeySet& shardReplicators) {
			auto count = io::Read16(input);
			while (count--) {
				Key replicatorKey;
				io::Read(input, replicatorKey);
				shardReplicators.insert(replicatorKey);
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
		io::Write16(output, downloadEntry.downloadApprovalCountLeft());

		io::Write(output, downloadEntry.getQueuePrevious());
		io::Write(output, downloadEntry.getQueueNext());
		io::Write(output, downloadEntry.getLastDownloadApprovalInitiated());
		io::Write8(output, downloadEntry.isFinishPublished());
		io::Write8(output, downloadEntry.downloadApprovalInitiationEvent().has_value());
		if (downloadEntry.downloadApprovalInitiationEvent().has_value()) {
			io::Write(output, *downloadEntry.downloadApprovalInitiationEvent());
		}

		SaveListOfPublicKeys(output, downloadEntry.listOfPublicKeys());
		SaveShardReplicators(output, downloadEntry.shardReplicators());
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

		entry.setDownloadApprovalCountLeft(io::Read16(input));

		Key queuePrevious;
		input.read(queuePrevious);
		entry.setQueuePrevious(queuePrevious);

		Key queueNext;
		input.read(queueNext);
		entry.setQueueNext(queueNext);

		entry.setLastDownloadApprovalInitiated(Timestamp(io::Read64(input)));
		entry.setFinishPublished(io::Read8(input));

		bool hasApprovalEvent = io::Read8(input);
		if (hasApprovalEvent) {
			Hash256 event;
			input.read(event);
			entry.downloadApprovalInitiationEvent() = event;
		}

		LoadListOfPublicKeys(input, entry.listOfPublicKeys());
		LoadShardReplicators(input, entry.shardReplicators());
		LoadCumulativePayments(input, entry.cumulativePayments());

		return entry;
	}
}}
