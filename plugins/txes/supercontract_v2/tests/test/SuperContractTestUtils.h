/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/SuperContractEntry.h"
#include "src/state/DriveContractEntry.h"
#include "src/model/SuperContractEntityType.h"
#include "tests/test/nodeps/Random.h"
#include "catapult/model/EntityPtr.h"
#include "catapult/utils/MemoryUtils.h"
#include "catapult/model/EntityBody.h"

namespace catapult { namespace test {

	state::SuperContractEntry CreateSuperContractEntry(Key key = test::GenerateRandomByteArray<Key>());

	state::DriveContractEntry CreateDriveContractEntry(Key key = test::GenerateRandomByteArray<Key>());

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
	/// Creates a automatic executions payment transaction.
	template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateAutomaticExecutionsPaymentTransaction() {
		auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_AutomaticExecutionsPaymentTransaction);
		pTransaction->ContractKey = test::GenerateRandomByteArray<Key>();
		pTransaction->AutomaticExecutionsNumber = test::Random32();
		return pTransaction;
	}
}}