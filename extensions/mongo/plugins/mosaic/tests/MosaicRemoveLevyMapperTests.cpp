/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/MosaicRemoveLevyMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "plugins/txes/mosaic/src/model/MosaicModifyLevyTransaction.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS MosaicLevyMapperTests
	
	TEST(TEST_CLASS, CanCreateMosaicRemoveLevyPlugin) {
		// Act:
		auto pPlugin = CreateMosaicRemoveLevyTransactionMongoPlugin();
		
		// Assert:
		EXPECT_EQ(model::Entity_Type_Mosaic_Remove_Levy, pPlugin->type());
	}
	
	TEST(TEST_CLASS, CanStreamMosaicRemoveLevy) {
		// Arrange:
		auto pPlugin = CreateMosaicRemoveLevyTransactionMongoPlugin();
		
		model::MosaicModifyLevyTransaction transaction;
		transaction.MosaicId = UnresolvedMosaicId (123);
		
		// Act:
		mappers::bson_stream::document builder;
		pPlugin->streamTransaction(builder, transaction);
		auto dbReceipt = builder << bsoncxx::builder::stream::finalize;
		
		// Assert:
		auto view = dbReceipt.view();
		EXPECT_EQ(1u, test::GetFieldCount(view));
				
		EXPECT_EQ(MosaicId(123), MosaicId(test::GetUint64(view, "mosaicId")));
	}
}}}
