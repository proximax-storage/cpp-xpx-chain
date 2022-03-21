/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/EntityBody.h"
#include "src/model/StreamingEntityType.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/test/nodeps/Random.h"

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

	/// Creates a stream start modification transaction.
	template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateStreamStartTransaction() {
		uint8_t folderNameSize = 255u;
		auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_StreamStart, folderNameSize);
		pTransaction->DriveKey = test::GenerateRandomByteArray<Key>();
		pTransaction->ExpectedUploadSizeMegabytes = test::Random();
		pTransaction->FolderNameSize = folderNameSize;
		test::FillWithRandomData(MutableRawBuffer(pTransaction->FolderNamePtr(), folderNameSize));
		return pTransaction;
	}

	/// Creates a stream finish modification transaction.
	template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateStreamFinishTransaction() {
		auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_StreamFinish);
		pTransaction->DriveKey = test::GenerateRandomByteArray<Key>();
		pTransaction->StreamId = test::GenerateRandomByteArray<Hash256>();
		pTransaction->ActualUploadSizeMegabytes = test::Random();
		return pTransaction;
	}

	/// Creates a stream payment modification transaction.
	template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateStreamPaymentTransaction() {
		auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_StreamPayment);
		pTransaction->DriveKey = test::GenerateRandomByteArray<Key>();
		pTransaction->StreamId = test::GenerateRandomByteArray<Hash256>();
		pTransaction->AdditionalUploadSizeMegabytes = test::Random();
		return pTransaction;
	}
}}