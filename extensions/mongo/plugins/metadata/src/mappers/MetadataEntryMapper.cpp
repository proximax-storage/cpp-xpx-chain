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
		void StreamMetadataField(bson_stream::array_context& context, const state::MetadataV1Field& field) {
			auto keyContext = context
					<< bson_stream::open_document
					<< "key" << field.MetadataKey
					<< "value" << field.MetadataValue;

			keyContext << bson_stream::close_document;
		}
	}

	bsoncxx::document::value ToDbModel(const state::MetadataV1Entry& metadataEntry) {
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

	state::MetadataV1Entry ToMetadataEntry(const bsoncxx::document::view& document) {
		auto dbMetadataEntry = document["metadata"];
		std::vector<uint8_t> raw;
		model::MetadataV1Type type;
		DbBinaryToStdContainer(raw, dbMetadataEntry["metadataId"].get_binary());
		type = model::MetadataV1Type(ToUint8(dbMetadataEntry["metadataType"].get_int32()));
		state::MetadataV1Entry metadataEntry(raw, type);

		auto dbFields = dbMetadataEntry["fields"].get_array().value;
		for (const auto& dbField : dbFields) {
			std::string key = std::string(dbField["key"].get_utf8().value);
			std::string value = std::string(dbField["value"].get_utf8().value);

			metadataEntry.fields().emplace_back(state::MetadataV1Field{ key, value, Height(0) });
		}

		return metadataEntry;
	}

	// endregion
}}}
