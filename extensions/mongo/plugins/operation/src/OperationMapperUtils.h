/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamBasicOperationTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		auto array = builder << "mosaics" << bson_stream::open_array;
		auto pMosaic = transaction.MosaicsPtr();
		for (auto i = 0u; i < transaction.MosaicCount; ++i, ++pMosaic)
			StreamMosaic(array, pMosaic->MosaicId, pMosaic->Amount);

		array << bson_stream::close_array;
	}

	template<typename TTransaction>
	void StreamBasicStartOperationTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		StreamBasicOperationTransaction(builder, transaction);
		builder << "duration" << ToInt64(transaction.Duration);
	}
}}}
