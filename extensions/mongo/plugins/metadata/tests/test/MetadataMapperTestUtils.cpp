/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MetadataMapperTestUtils.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {

	// region metadata entry related

	namespace {
		void AssertFields(const std::vector<state::MetadataField>& fields, const bsoncxx::document::view& dbFields) {
			ASSERT_EQ(fields.size(), test::GetFieldCount(dbFields));

			for (const auto& dbField : dbFields) {
				std::string key = std::string(dbField["key"].get_utf8().value);
				std::string value = std::string(dbField["value"].get_utf8().value);
				auto iter = std::find_if(fields.begin(), fields.end(), [&key, &value](const state::MetadataField& field) {
					return (field.MetadataKey == key) && (field.MetadataValue == value);
				});

				EXPECT_TRUE(iter != fields.end());
			}
		}
	}

	void AssertEqualMetadataData(const state::MetadataEntry& entry, const bsoncxx::document::view& dbMetadata) {
		std::vector<uint8_t> raw;
		mongo::mappers::DbBinaryToStdContainer(raw, dbMetadata["metadataId"].get_binary());
		EXPECT_EQ(entry.raw(), raw);
		EXPECT_EQ(entry.type(), model::MetadataType(GetUint32(dbMetadata, "metadataType")));

		AssertFields(entry.fields(), dbMetadata["fields"].get_array().value);
	}

	// endregion
}}
