#include "LevyEntryMapperTestUtils.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "plugins/txes/mosaic/src/state/LevyEntry.h"
#include "catapult/utils/Casting.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "tests/TestHarness.h"
#include "mongo/src/mappers/MapperUtils.h"

namespace catapult { namespace test {
	using namespace mongo::mappers;
	
	void AssertEqualLevyData(const state::LevyEntry& entry, const bsoncxx::document::view& dbMosaic) {
		const auto& levy = entry.levy();
		auto dbLevy = dbMosaic["levy"].get_document().view();
		
		model::LevyType type = static_cast<model::LevyType>(static_cast<uint32_t>(dbLevy["type"].get_int32()));
		auto mosaicId = GetValue64<MosaicId>(dbLevy["mosaicId"]);
		
		UnresolvedAddress address;
		DbBinaryToModelArray(address, dbLevy["recipient"].get_binary());
		Amount fee = GetValue64<Amount>(dbLevy["fee"]);
		
		EXPECT_EQ(levy.Type, type);
		EXPECT_EQ(levy.Recipient, address);
		EXPECT_EQ(levy.MosaicId, mosaicId);
		EXPECT_EQ(levy.Fee, fee);
	}
}}
