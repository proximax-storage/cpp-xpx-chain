/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#include "LevyMapperUtils.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "plugins/txes/mosaic/src/state/LevyEntry.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace mongo { namespace plugins { namespace levy {
	using namespace catapult::mongo::mappers;
	
	state::LevyEntryData ReadLevy(const bsoncxx::document::view& dbLevy) {
		model::LevyType type = static_cast<model::LevyType>(static_cast<uint32_t>(dbLevy["type"].get_int32()));
		auto mosaicId = GetValue64<MosaicId>(dbLevy["mosaicId"]);
		
		catapult::Address address;
		DbBinaryToModelArray(address, dbLevy["recipient"].get_binary());
		
		Amount fee = GetValue64<Amount>(dbLevy["fee"]);
		return state::LevyEntryData(type, address, mosaicId, fee);
	}
	
}}}}
