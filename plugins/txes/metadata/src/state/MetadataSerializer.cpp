/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MetadataSerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/Casting.h"
#include <map>
#include <vector>

namespace catapult { namespace state {

	// region MetadataSerializer

	void MetadataSerializer::Save(const MetadataEntry& metadataEntry, io::OutputStream& output) {
        io::Write32(output, metadataEntry.getVersion());

		// write identifying information
		io::Write8(output, metadataEntry.raw().size());
		io::Write(output, metadataEntry.raw());
		io::Write8(output, utils::to_underlying_type(metadataEntry.type()));

		io::Write8(output, metadataEntry.fields().size());
		for (const auto& field : metadataEntry.fields()) {
			io::Write(output, field.RemoveHeight);
			io::Write8(output, field.MetadataKey.size());
			io::Write(output, RawBuffer((const uint8_t*)field.MetadataKey.data(), field.MetadataKey.size()));
			io::Write16(output, field.MetadataValue.size());
			io::Write(output, RawBuffer((const uint8_t*)field.MetadataValue.data(), field.MetadataValue.size()));
		}
	}

	MetadataEntry MetadataSerializer::Load(io::InputStream& input) {
        // read version
        VersionType version = io::Read32(input);

		uint8_t rawSize = io::Read8(input);
		std::vector<uint8_t> raw(rawSize);
		io::Read(input, raw);

		model::MetadataType type;
		type = static_cast<model::MetadataType>(io::Read8(input));

		MetadataEntry metadataEntry(raw, type, version);

		uint8_t fieldsSize = io::Read8(input);

		for (auto i = 0u; i < fieldsSize; ++i) {
			metadataEntry.fields().emplace_back(MetadataField());
			auto& field = metadataEntry.fields().back();
			io::Read(input, field.RemoveHeight);

			uint8_t MetadataKeySize = io::Read8(input);
			field.MetadataKey.resize(MetadataKeySize);
			io::Read(input, MutableRawBuffer((uint8_t*)field.MetadataKey.data(), field.MetadataKey.size()));


			uint16_t MetadataValueSize = io::Read16(input);
			field.MetadataValue.resize(MetadataValueSize);
			io::Read(input, MutableRawBuffer((uint8_t*)field.MetadataValue.data(), field.MetadataValue.size()));
		}

		return metadataEntry;
	}

	// endregion
}}
