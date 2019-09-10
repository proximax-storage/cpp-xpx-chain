/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/model/MetadataTypes.h"
#include "catapult/utils/MemoryUtils.h"
#include "catapult/model/EntityPtr.h"


namespace catapult { namespace test {

    /// Generates a metadata modification with \a type, \a keySize and \a valueSize.
    model::UniqueEntityPtr<model::MetadataModification> CreateModification(
        model::MetadataModificationType type, uint8_t keySize, uint16_t valueSize);

    /// Creates a metadata transaction with \a modifications.
    template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateTransaction(std::initializer_list<model::MetadataModification*> modifications) {
        uint32_t entitySize = sizeof(TTransaction);
        for (auto pModification : modifications) {
            entitySize += pModification->Size;
        }
        auto pTransaction = utils::MakeUniqueWithSize<TTransaction>(entitySize);
		pTransaction->Version = MakeVersion(model::NetworkIdentifier::Mijin_Test, 1);
        pTransaction->Size = entitySize;

        auto* pData = reinterpret_cast<uint8_t*>(pTransaction.get() + 1);
        for (auto pModification : modifications) {
            memcpy(pData, pModification, pModification->Size);
            pData += pModification->Size;
        }

        return pTransaction;
    }
}}
