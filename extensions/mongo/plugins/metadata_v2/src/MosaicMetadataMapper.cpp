/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
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

#include "MosaicMetadataMapper.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "plugins/txes/metadata_v2/src/model/MosaicMetadataTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Stream(bson_stream::document& builder, const TTransaction& transaction) {
			builder
					<< "targetKey" << ToBinary(transaction.TargetKey)
					<< "scopedMetadataKey" << static_cast<int64_t>(transaction.ScopedMetadataKey)
					<< "targetMosaicId" << ToInt64(transaction.TargetMosaicId)
					<< "valueSizeDelta" << transaction.ValueSizeDelta
					<< "valueSize" << transaction.ValueSize;

			if (0 < transaction.ValueSize)
				builder << "value" << ToBinary(transaction.ValuePtr(), transaction.ValueSize);
		}
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(MosaicMetadata, Stream)
}}}
