/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
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

#include "mongo/plugins/lock_fund/src/LockFundReceiptMapper.h"
#include "mongo/src/MongoReceiptPluginFactory.h"
#include "mongo/src/mappers/ReceiptMapper.h"
#include "plugins/txes/lock_fund/src/model/LockFundTotalStakedReceipt.h"
#include "plugins/txes/lock_fund/src/model/LockFundReceiptType.h"
#include "catapult/model/Receipt.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	std::unique_ptr<MongoReceiptPlugin> CreateLockFundReceiptMongoPlugin() {
		return MongoReceiptPluginFactory::Create<model::TotalStakedReceipt>(
				model::Receipt_Type_Total_Staked,
				[](bson_stream::document& builder, const model::TotalStakedReceipt& receipt){
					builder << "amount" << ToInt64(receipt.Amount);
				});
	}
}}}
