/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CatapultConfigEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/exceptions.h"

namespace catapult { namespace state {

	void CatapultConfigEntrySerializer::Save(const CatapultConfigEntry& entry, io::OutputStream& output) {
		// write version
		io::Write32(output, 1);

		io::Write64(output, entry.height().unwrap());

		const auto& blockChainConfig = entry.blockChainConfig();
		io::Write16(output, blockChainConfig.size());
		io::Write(output, RawBuffer((const uint8_t*)blockChainConfig.data(), blockChainConfig.size()));

		const auto& supportedEntityVersions = entry.supportedEntityVersions();
		io::Write16(output, supportedEntityVersions.size());
		io::Write(output, RawBuffer((const uint8_t*)supportedEntityVersions.data(), supportedEntityVersions.size()));
	}

	CatapultConfigEntry CatapultConfigEntrySerializer::Load(io::InputStream& input) {
        // read version
        VersionType version = io::Read32(input);
        if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of CatapultConfigEntry", version);

		auto height = Height{io::Read64(input)};

		std::string blockChainConfig;
		uint16_t blockChainConfigSize = io::Read16(input);
		blockChainConfig.resize(blockChainConfigSize);
		io::Read(input, MutableRawBuffer((uint8_t*)blockChainConfig.data(), blockChainConfig.size()));

		std::string supportedEntityVersions;
		uint16_t supportedEntityVersionsSize = io::Read16(input);
		supportedEntityVersions.resize(supportedEntityVersionsSize);
		io::Read(input, MutableRawBuffer((uint8_t*)supportedEntityVersions.data(), supportedEntityVersions.size()));

		return state::CatapultConfigEntry(height, blockChainConfig, supportedEntityVersions);
	}
}}
