/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "catapult/model/EntityBody.h"
#include "catapult/model/Mosaic.h"
#include "catapult/utils/MemoryUtils.h"
#include "src/model/ContractEntityType.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"

namespace catapult { namespace test {

    /// Creates a transaction.
	template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateTransaction(model::EntityType type, size_t additionalSize = 0) {
		uint32_t entitySize = sizeof(TTransaction) + additionalSize;
		auto pTransaction = utils::MakeUniqueWithSize<TTransaction>(entitySize);
		pTransaction->Signer = test::GenerateRandomByteArray<Key>();
		pTransaction->Version = model::MakeVersion(model::NetworkIdentifier::Mijin_Test, 1);
		pTransaction->Type = type;
		pTransaction->Size = entitySize;

		return pTransaction;
	}

    /// Creates a deploy transaction.
    template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateDeployTransaction(uint8_t servicePaymentCount) {
        uint8_t fileNameSize = 255u;
        uint8_t functionNameSize = 255u;
        uint8_t actualArgumentsSize = 255u;
        uint8_t automatedExecutionFileNameSize = 255u;
        uint8_t automatedExecutionFunctionNameSize = 255u;

        auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_Deploy, 
                fileNameSize + functionNameSize + actualArgumentsSize 
                + servicePaymentCount * sizeof(Mosaic) 
                + automatedExecutionFileNameSize + automatedExecutionFunctionNameSize);
        pTransaction->DriveKey = test::GenerateRandomByteArray<Key>();
        pTransaction->FileNameSize = fileNameSize;
        test::FillWithRandomData(MutableRawBuffer(pTransaction->FileNamePtr(), fileNameSize));
        pTransaction->FunctionNameSize = functionNameSize;
        test::FillWithRandomData(MutableRawBuffer(pTransaction->FunctionNamePtr(), functionNameSize));
        pTransaction->ActualArgumentsSize = actualArgumentsSize;
        test::FillWithRandomData(MutableRawBuffer(pTransaction->ActualArgumentsPtr(), actualArgumentsSize));
        pTransaction->AutomatedExecutionFileNameSize = automatedExecutionFileNameSize;
        test::FillWithRandomData(MutableRawBuffer(pTransaction->AutomatedExecutionFileNamePtr(), automatedExecutionFileNameSize));
        pTransaction->AutomatedExecutionFunctionNameSize = automatedExecutionFunctionNameSize;
        test::FillWithRandomData(MutableRawBuffer(pTransaction->AutomatedExecutionFunctionNamePtr(), automatedExecutionFunctionNameSize));

        return pTransaction;
    }
}}