/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <random>
#include "catapult/dbrb/View.h"
#include "catapult/model/EntityBody.h"
#include "src/cache/DbrbViewCache.h"
#include "src/cache/DbrbViewCacheStorage.h"
#include "src/model/DbrbEntityType.h"
#include "src/model/DbrbNotifications.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/test/nodeps/Random.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {

	/// Generates random View with given \a length.
	dbrb::View GenerateRandomView(const size_t& length = 4u);

    /// Creates test view sequence entry.
    state::DbrbProcessEntry CreateDbrbProcessEntry(
		const dbrb::ProcessId& processId = test::GenerateRandomByteArray<dbrb::ProcessId>(),
		const Timestamp& expirationTime = test::GenerateRandomValue<Timestamp>()
    );

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

    /// Creates an Install message transaction.
    template<typename TTransaction>
    model::UniqueEntityPtr<TTransaction> CreateAddDbrbProcessTransaction() {
        return CreateTransaction<TTransaction>(model::Entity_Type_AddDbrbProcess);
    }

	void AssertEqualDbrbProcessEntry(const state::DbrbProcessEntry& entry1, const state::DbrbProcessEntry& entry2);
}}
