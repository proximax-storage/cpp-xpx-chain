/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ReplicatorEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	namespace {
		void SaveDrives(io::OutputStream& output, const DrivesMap& drives) {
			io::Write16(output, utils::checked_cast<size_t, uint16_t>(drives.size()));
			for (const auto& drivePair : drives) {
				io::Write(output, drivePair.first);
				io::Write(output, drivePair.second.LastApprovedDataModificationId);
				io::Write64(output, drivePair.second.InitialDownloadWorkMegabytes);
				io::Write64(output, drivePair.second.LastCompletedCumulativeDownloadWorkBytes);
			}
		}

		void LoadDrives(io::InputStream& input, DrivesMap& drives) {
			auto count = io::Read16(input);
			while (count--) {
				Key drive;
				io::Read(input, drive);

				DriveInfo info;
				io::Read(input, info.LastApprovedDataModificationId);
				info.InitialDownloadWorkMegabytes = io::Read64(input);
				info.LastCompletedCumulativeDownloadWorkBytes = io::Read64(input);

				drives.emplace(drive, info);
			}
		}

		void SaveDownloadChannels(io::OutputStream& output, const std::set<Hash256>& downloadChannels) {
			io::Write16(output, utils::checked_cast<size_t, uint16_t>(downloadChannels.size()));
			for (const auto& id : downloadChannels)
				io::Write(output, id);
		}

		void LoadDownloadChannels(io::InputStream& input, std::set<Hash256>& downloadChannels) {
			auto count = io::Read16(input);
			while (count--) {
				Hash256 id;
				io::Read(input, id);
				downloadChannels.insert(std::move(id));
			}
		}

		void SaveReplicatorsSetNode(io::OutputStream& output, const AVLTreeNode& node) {
			io::Write(output, node.Left);
			io::Write(output, node.Right);
			io::Write16(output, node.Height);
			io::Write32(output, node.Size);
		}

		void LoadReplicatorsSetNode(io::InputStream& input, AVLTreeNode& node) {
			Key left;
			io::Read(input, left);
			Key right;
			io::Read(input, right);
			uint16_t height = io::Read16(input);
			uint32_t size = io::Read32(input);
			node = AVLTreeNode{left, right, height, size};
		}
	}

	void ReplicatorEntrySerializer::Save(const ReplicatorEntry& replicatorEntry, io::OutputStream& output) {

		io::Write32(output, replicatorEntry.version());
		io::Write(output, replicatorEntry.key());

		SaveReplicatorsSetNode(output, replicatorEntry.replicatorsSetNode());

		SaveDrives(output, replicatorEntry.drives());
		SaveDownloadChannels(output, replicatorEntry.downloadChannels());
	}

	ReplicatorEntry ReplicatorEntrySerializer::Load(io::InputStream& input) {

		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of ReplicatorEntry", version);

		Key key;
		input.read(key);
		state::ReplicatorEntry entry(key);
		entry.setVersion(version);

		LoadReplicatorsSetNode(input, entry.replicatorsSetNode());

		LoadDrives(input, entry.drives());
		LoadDownloadChannels(input, entry.downloadChannels());

		return entry;
	}
}}
