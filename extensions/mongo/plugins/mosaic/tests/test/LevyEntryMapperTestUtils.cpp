/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "LevyEntryMapperTestUtils.h"
#include "src/utils/LevyMapperUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {
	using namespace mongo::mappers;
	using namespace mongo::plugins;
	
	void AssertEqualLevyData(const state::LevyEntry& entry, const bsoncxx::document::view& dbMosaic) {
		const auto& levy = entry.levy();
		auto dbLevy = dbMosaic["levy"].get_document().view();
		
		model::LevyType type = static_cast<model::LevyType>(static_cast<uint32_t>(dbLevy["type"].get_int32()));
		auto mosaicId = GetValue64<MosaicId>(dbLevy["mosaicId"]);
		
		Address address;
		DbBinaryToModelArray(address, dbLevy["recipient"].get_binary());
		Amount fee = GetValue64<Amount>(dbLevy["fee"]);
		
		EXPECT_EQ(levy->Type, type);
		EXPECT_EQ(levy->Recipient, address);
		EXPECT_EQ(levy->MosaicId, mosaicId);
		EXPECT_EQ(levy->Fee, fee);
	}
	
	void AssetEqualHistory(state::LevyEntry& entry, const bsoncxx::document::view& dbMosaic) {
		auto historyEntry = dbMosaic["history"].get_array().value;
		
		auto historyCount = 0;
		auto history = entry.updateHistory();
		for (const auto& dbHistoryItem : historyEntry) {
			auto doc = dbHistoryItem.get_document().view();
			auto height = Height{static_cast<uint64_t>(doc["height"].get_int64())};
			auto lhs = levy::ReadLevy(doc);
			
			EXPECT_EQ(history[historyCount].first, height);
			
			auto rhs = history[historyCount].second;
			EXPECT_EQ(rhs.Type, lhs.Type);
			EXPECT_EQ(rhs.Recipient, lhs.Recipient);
			EXPECT_EQ(rhs.MosaicId, lhs.MosaicId);
			EXPECT_EQ(rhs.Fee, lhs.Fee);
			
			historyCount++;
		}
		
		EXPECT_EQ(entry.updateHistory().size(), historyCount);
	}
}}
