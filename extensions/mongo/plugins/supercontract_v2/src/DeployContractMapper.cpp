/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DeployContractMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "src/model/DeployContractTransaction.h"
#include "CommonSupercontractStreams.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

    template<typename TTransaction>
    void StreamDeployContractTransaction(bson_stream::document& builder, const TTransaction& transaction) {
        builder << "driveKey" << ToBinary(transaction.DriveKey)
                << "assignee" << ToBinary(transaction.Assignee)
                << "automaticExecutionCallPayment" << ToInt64(transaction.AutomaticExecutionCallPayment)
                << "automaticDownloadCallPayment" << ToInt64(transaction.AutomaticDownloadCallPayment)
				<< "automaticExecutionsNumber" << static_cast<int32_t>(transaction.AutomaticExecutionsNumber);
		if (transaction.AutomaticExecutionsFileNameSize) {
			builder << "automaticExecutionsFileName" << ToBinary(transaction.AutomaticExecutionsFileNamePtr(), transaction.AutomaticExecutionsFileNameSize);
		}
		if (transaction.AutomaticExecutionsFunctionNameSize) {
			builder << "automaticExecutionsFunctionName" << ToBinary(transaction.AutomaticExecutionsFunctionNamePtr(), transaction.AutomaticExecutionsFunctionNameSize);
		}

        StreamManualCall(builder,
						 transaction.FileNamePtr(),
						 transaction.FileNameSize,
						 transaction.FunctionNamePtr(),
						 transaction.FunctionNameSize,
						 transaction.ActualArgumentsPtr(),
						 transaction.ActualArgumentsSize,
						 transaction.ServicePaymentsPtr(),
						 transaction.ServicePaymentsCount,
						 transaction.ExecutionCallPayment,
						 transaction.DownloadCallPayment);
    }

    DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(DeployContract, StreamDeployContractTransaction)
}}}
