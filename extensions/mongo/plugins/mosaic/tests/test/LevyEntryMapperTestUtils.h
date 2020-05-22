#pragma once
#include <bsoncxx/json.hpp>
#include <plugins/txes/mosaic/src/state/LevyEntry.h>

namespace catapult { namespace state { class MosaicEntry; } }

namespace catapult { namespace test {
		
	/// Verifies that db mosaic (\a dbMosaic) and model mosaic \a entry are equivalent.
	void AssertEqualLevyData(const state::LevyEntry& entry, const bsoncxx::document::view& dbMosaic);
	
	/// checks for history entries against mongo
	void AssetEqualHistory(state::LevyEntry& entry, const bsoncxx::document::view& dbMosaic);
}}
