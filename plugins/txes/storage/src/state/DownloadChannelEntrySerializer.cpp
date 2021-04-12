/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DownloadChannelEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	void DownloadChannelEntrySerializer::Save(const DownloadChannelEntry& downloadEntry, io::OutputStream& output) {
		auto version = downloadEntry.version();

		io::Write32(output, version);

		io::Write(output, downloadEntry.id());
		io::Write(output, downloadEntry.consumer());
		io::Write(output, downloadEntry.drive());
		io::Write(output, downloadEntry.transactionFee());
		io::Write(output, downloadEntry.storageUnits());
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

		entry.setTransactionFee(Amount(io::Read64(input)));
		entry.setStorageUnits(Amount(io::Read64(input)));

		return entry;
	}
}}
