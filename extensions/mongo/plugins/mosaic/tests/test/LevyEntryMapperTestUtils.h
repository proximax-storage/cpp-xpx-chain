/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <bsoncxx/json.hpp>
#include "plugins/txes/mosaic/src/state/LevyEntry.h"

namespace catapult { namespace state { class MosaicEntry; } }

namespace catapult { namespace test {
		
	/// Verifies that db mosaic (\a dbMosaic) and model mosaic \a entry are equivalent.
	void AssertEqualLevyData(const state::LevyEntry& entry, const bsoncxx::document::view& dbMosaic);
	
	/// checks for history entries against mongo
	void AssetEqualHistory(state::LevyEntry& entry, const bsoncxx::document::view& dbMosaic);
}}
