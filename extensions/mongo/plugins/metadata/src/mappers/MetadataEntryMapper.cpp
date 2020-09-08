/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MetadataEntryMapper.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	namespace {
		void StreamMetadataField(bson_stream::array_context& context, const state::MetadataField& field) {
			auto keyContext = context
					<< bson_stream::open_document
					<< "key" << field.MetadataKey
					<< "value" << field.MetadataValue;

			keyContext << bson_stream::close_document;
		}
	}

	bsoncxx::document::value ToDbModel(const state::MetadataEntry& metadataEntry) {
		bson_stream::document builder;
		auto doc = builder << "metadata" << bson_stream::open_document
				<< "metadataId" << ToBinary(metadataEntry.raw().data(), metadataEntry.raw().size())
				<< "metadataType" << static_cast<int8_t>(metadataEntry.type());

		auto fieldArray = builder << "fields" << bson_stream::open_array;
		for (const auto& field : metadataEntry.fields())
			if (field.RemoveHeight.unwrap() == 0)
				StreamMetadataField(fieldArray, field);

		fieldArray << bson_stream::close_array;

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	state::MetadataEntry ToMetadataEntry(const bsoncxx::document::view& document) {
		auto dbMetadataEntry = document["metadata"];
		std::vector<uint8_t> raw;
		model::MetadataType type;
		DbBinaryToStdContainer(raw, dbMetadataEntry["metadataId"].get_binary());
		type = model::MetadataType(ToUint8(dbMetadataEntry["metadataType"].get_int32()));
		state::MetadataEntry metadataEntry(raw, type);

		auto dbFields = dbMetadataEntry["fields"].get_array().value;
		for (const auto& dbField : dbFields) {
			std::string key = dbField["key"].get_utf8().value.to_string();
			std::string value = dbField["value"].get_utf8().value.to_string();

			metadataEntry.fields().emplace_back(state::MetadataField{ key, value, Height(0) });
		}

		return metadataEntry;
	}

	// endregion
}}}
