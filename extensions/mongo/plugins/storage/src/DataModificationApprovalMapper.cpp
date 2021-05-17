/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DataModificationApprovalMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/storage/src/model/DataModificationApprovalTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamDataModificationApprovalTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "driveKey" << ToBinary(transaction.DriveKey);
		builder << "dataModificationId" << ToBinary(transaction.DataModificationId);
		builder << "fileStructureCDI" << ToBinary(transaction.FileStructureCDI);
		builder << "fileStructureSize" << static_cast<int64_t>(transaction.FileStructureSize);
		builder << "usedDriveSize" << static_cast<int64_t>(transaction.UsedDriveSize);
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(DataModificationApproval, StreamDataModificationApprovalTransaction)
}}}
