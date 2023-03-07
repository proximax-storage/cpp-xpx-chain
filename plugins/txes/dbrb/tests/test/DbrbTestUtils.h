/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <random>
#include "catapult/model/EntityBody.h"
#include "src/cache/ViewSequenceCache.h"
#include "src/cache/ViewSequenceCacheStorage.h"
#include "src/model/DbrbEntityType.h"
#include "src/model/DbrbNotifications.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/test/nodeps/Random.h"

namespace catapult { namespace test {

	/// Generates random View with given \a length.
	dbrb::View GenerateRandomView(const size_t& length = 4u);

	/// Generates random Sequence with given \a initialView, \a length and \a maxStep.
	dbrb::Sequence GenerateRandomSequence(
		const dbrb::View& initialView = {},
		const size_t& length = 3u,
		const uint8_t& maxStep = 2u
	);

    /// Creates test view sequence entry.
    state::ViewSequenceEntry CreateViewSequenceEntry(
		const Hash256& hash = test::GenerateRandomByteArray<Hash256>(),
		const dbrb::Sequence& sequence = GenerateRandomSequence()
    );

	/// Creates test message hash entry.
	state::MessageHashEntry CreateMessageHashEntry(
		const Hash256& hash = test::GenerateRandomByteArray<Hash256>()
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
    model::UniqueEntityPtr<TTransaction> CreateInstallMessageTransaction() {
        auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_InstallMessage);
		pTransaction->MessageHash = test::GenerateRandomByteArray<Hash256>();
        return pTransaction;
    }
}}
