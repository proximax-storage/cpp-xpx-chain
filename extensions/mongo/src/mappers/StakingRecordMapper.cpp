/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#include "catapult/state/StakingRecord.h"
#include "StakingRecordMapper.h"
#include "MapperUtils.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace mongo { namespace mappers {

	// region ToDbModel

	bsoncxx::document::value ToDbModel(const state::StakingRecord& stakingRecord) {
		// account metadata
		bson_stream::document builder;
		builder << "stakingAccount" << bson_stream::open_document << "address" << ToBinary(stakingRecord.Address)
				<< "publicKey" << ToBinary(stakingRecord.PublicKey)
				<< "registryHeight" << ToInt64(stakingRecord.RegistryHeight)
				<< "totalStaked" << ToInt64(stakingRecord.TotalStaked);
		builder << bson_stream::close_document;
		return builder << bson_stream::finalize;
	}

	// endregion
}}}
