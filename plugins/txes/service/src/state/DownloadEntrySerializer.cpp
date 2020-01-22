/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DownloadEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/exceptions.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	void DownloadEntrySerializer::Save(const DownloadEntry& entry, io::OutputStream& output) {
		// write version
		io::Write32(output, 1);

		io::Write(output, entry.driveKey());
		const auto& fileRecipients = entry.fileRecipients();
		io::Write32(output, utils::checked_cast<size_t, uint32_t>(fileRecipients.size()));
		for (const auto& fileRecipientPair : fileRecipients) {
			io::Write(output, fileRecipientPair.first);
			const auto& downloads = fileRecipientPair.second;
			io::Write32(output, utils::checked_cast<size_t, uint32_t>(downloads.size()));
			for (const auto& downloadPair : downloads) {
				io::Write(output, downloadPair.first);
				const auto& fileHashes = downloadPair.second;
				io::Write16(output, utils::checked_cast<size_t, uint16_t>(fileHashes.size()));
				for (const auto& fileHash : fileHashes)
					io::Write(output, fileHash);
			}
		}
	}

	DownloadEntry DownloadEntrySerializer::Load(io::InputStream& input) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of DownloadEntry", version);

		Key driveKey;
		io::Read(input, driveKey);
		DownloadEntry entry(driveKey);
		auto fileRecipientCount = io::Read32(input);
		while (fileRecipientCount--) {
			Key fileRecipientKey;
			io::Read(input, fileRecipientKey);
			DownloadMap downloads;
			auto downloadCount = io::Read32(input);
			while (downloadCount--) {
				Hash256 operationToken;
				io::Read(input, operationToken);
				std::set<Hash256> fileHashes;
				auto fileCount = io::Read16(input);
				while (fileCount--) {
					Hash256 fileHash;
					io::Read(input, fileHash);
					fileHashes.emplace(fileHash);
				}
				downloads.emplace(operationToken, fileHashes);
			}
			entry.fileRecipients().emplace(fileRecipientKey, downloads);
		}

		return entry;
	}
}}
