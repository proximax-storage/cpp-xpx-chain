/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MetadataV1Serializer.h"
#include "catapult/io/PodIoUtils.h"

namespace catapult { namespace state {

	// region MetadataSerializer

	void MetadataV1Serializer::Save(const MetadataV1Entry& metadataEntry, io::OutputStream& output) {
		// write version
		io::Write32(output, 1);

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

	MetadataV1Entry MetadataV1Serializer::Load(io::InputStream& input) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of MetadataEntry", version);

		uint8_t rawSize = io::Read8(input);
		std::vector<uint8_t> raw(rawSize);
		io::Read(input, raw);

		model::MetadataV1Type type;
		type = static_cast<model::MetadataV1Type>(io::Read8(input));

		MetadataV1Entry metadataEntry(raw, type);

		uint8_t fieldsSize = io::Read8(input);

		for (auto i = 0u; i < fieldsSize; ++i) {
			metadataEntry.fields().emplace_back(MetadataV1Field());
			auto& field = metadataEntry.fields().back();
			io::Read(input, field.RemoveHeight);

			uint8_t MetadataV1KeySize = io::Read8(input);
			field.MetadataKey.resize(MetadataV1KeySize);
			io::Read(input, MutableRawBuffer((uint8_t*)field.MetadataKey.data(), field.MetadataKey.size()));


			uint16_t MetadataValueSize = io::Read16(input);
			field.MetadataValue.resize(MetadataValueSize);
			io::Read(input, MutableRawBuffer((uint8_t*)field.MetadataValue.data(), field.MetadataValue.size()));
		}

		return metadataEntry;
	}

	// endregion
}}
