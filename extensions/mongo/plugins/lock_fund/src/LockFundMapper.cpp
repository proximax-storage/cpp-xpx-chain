/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "LockFundMapper.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "plugins/txes/lock_fund/src/model/LockFundTransferTransaction.h"
#include "plugins/txes/lock_fund/src/model/LockFundCancelUnlockTransaction.h"
using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	namespace {

		template<typename TTransaction>
		void StreamTransferUnlock(bson_stream::document& builder, const TTransaction& transaction) {
			builder
					<< "targetHeight" << ToInt64(transaction.TargetHeight);
		}

		void StreamMosaics(bson_stream::document& builder, const model::UnresolvedMosaic* pMosaic, size_t numMosaics) {
			auto mosaicsArray = builder << "mosaics" << bson_stream::open_array;
			for (auto i = 0u; i < numMosaics; ++i) {
				StreamMosaic(mosaicsArray, pMosaic->MosaicId, pMosaic->Amount);
				++pMosaic;
			}

			mosaicsArray << bson_stream::close_array;
		}

		template<typename TTransaction>
		void StreamTransfer(bson_stream::document& builder, const TTransaction& transaction) {
			builder << "duration" << ToInt64(transaction.Duration);
			builder << "action" << ToUint8((uint8_t)transaction.Action);
			StreamMosaics(builder, transaction.MosaicsPtr(), transaction.MosaicsCount);
		}
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(LockFundCancelUnlock, StreamTransferUnlock)
	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(LockFundTransfer, StreamTransfer)
}}}
