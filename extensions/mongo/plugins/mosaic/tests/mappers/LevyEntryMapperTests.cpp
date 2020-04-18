/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "src/mappers/LevyEntryMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "plugins/txes/mosaic/src/state/LevyEntry.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "plugins/txes/mosaic/tests/test/LevyTestUtils.h"
#include "tests/test/MosaicMapperTestUtils.h"
#include "tests/test/LevyEntryMapperTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS LevyEntryMapperTests
			
	// region ToDbModel
	
	namespace {
		state::LevyEntry CreateLevyEntry() {
			auto levy = test::CreateValidMosaicLevy();
			return state::LevyEntry(MosaicId(123), levy);
		}
	}
	
	TEST(TEST_CLASS, CanMapLevyEntry_ModelToDbModel) {
		// Arrange:
		auto entry = CreateLevyEntry();
		
		// Act:
		auto document = ToDbModel(entry);
		auto documentView = document.view();
		
		// Assert:
		EXPECT_EQ(1u, test::GetFieldCount(documentView));
		
		auto levyView = documentView["levy"].get_document().view();
		EXPECT_EQ(2u, test::GetFieldCount(levyView));
		test::AssertEqualLevyData(entry, levyView);
	}
	
	// endregion
	
	// region ToLevyEntry
	
	namespace {
		bsoncxx::document::value CreateDbLevyEntry() {
			return ToDbModel(CreateLevyEntry());
		}
	}
	
	TEST(TEST_CLASS, CanMapLevyEntry_DbModelToModel) {
		// Arrange:
		auto dbEntry = CreateDbLevyEntry();
		
		// Act:
		auto entry = ToLevyEntry(dbEntry);
		
		// Assert: only the mosaic field (not meta) is mapped to the model
		auto view = dbEntry.view();
		EXPECT_EQ(1u, test::GetFieldCount(view));
		
		auto mosaicView = view["levy"].get_document().view();
		EXPECT_EQ(2u, test::GetFieldCount(mosaicView));
		test::AssertEqualLevyData(entry, mosaicView);
	}
	
	// endregion
}}}
