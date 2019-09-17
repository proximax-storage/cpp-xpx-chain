/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NetworkConfigEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/exceptions.h"

namespace catapult { namespace state {

	void NetworkConfigEntrySerializer::Save(const NetworkConfigEntry& entry, io::OutputStream& output) {
		// write version
		io::Write32(output, 1);

		io::Write64(output, entry.height().unwrap());

		const auto& networkConfig = entry.networkConfig();
		io::Write16(output, networkConfig.size());
		io::Write(output, RawBuffer((const uint8_t*)networkConfig.data(), networkConfig.size()));

		const auto& supportedEntityVersions = entry.supportedEntityVersions();
		io::Write16(output, supportedEntityVersions.size());
		io::Write(output, RawBuffer((const uint8_t*)supportedEntityVersions.data(), supportedEntityVersions.size()));
	}

	NetworkConfigEntry NetworkConfigEntrySerializer::Load(io::InputStream& input) {
        // read version
        VersionType version = io::Read32(input);
        if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of NetworkConfigEntry", version);

		auto height = Height{io::Read64(input)};

		std::string networkConfig;
		uint16_t networkConfigSize = io::Read16(input);
		networkConfig.resize(networkConfigSize);
		io::Read(input, MutableRawBuffer((uint8_t*)networkConfig.data(), networkConfig.size()));

		std::string supportedEntityVersions;
		uint16_t supportedEntityVersionsSize = io::Read16(input);
		supportedEntityVersions.resize(supportedEntityVersionsSize);
		io::Read(input, MutableRawBuffer((uint8_t*)supportedEntityVersions.data(), supportedEntityVersions.size()));

		return state::NetworkConfigEntry(height, networkConfig, supportedEntityVersions);
	}
}}
