
#include "src/MosaicModifyLevyMapper.h"
#include "plugins/txes/mosaic/src/model/MosaicModifyLevyTransaction.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "tests/TestHarness.h"

#include "mongo/src/mappers/MapperUtils.h"
#include "catapult/constants.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS MosaicLevyMapperTests
	
	TEST(TEST_CLASS, CanCreateMosaicLevyPlugin) {
		// Act:
		auto pPlugin = CreateMosaicModifyLevyTransactionMongoPlugin();
		
		// Assert:
		EXPECT_EQ(model::Entity_Type_Mosaic_Modify_Levy, pPlugin->type());
	}
	
	TEST(TEST_CLASS, CanStreamMosaicLevy) {
		// Arrange:
		auto pPlugin = CreateMosaicModifyLevyTransactionMongoPlugin();
		
		model::MosaicModifyLevyTransaction transaction;
		transaction.MosaicId = UnresolvedMosaicId (123);
		
		// Act:
		mappers::bson_stream::document builder;
		pPlugin->streamTransaction(builder, transaction);
		auto dbReceipt = builder << bsoncxx::builder::stream::finalize;
		
		// Assert:
		auto view = dbReceipt.view();
		EXPECT_EQ(2u, test::GetFieldCount(view));
				
		EXPECT_EQ(MosaicId(123), MosaicId(test::GetUint64(view, "mosaicId")));
	}
}}}
