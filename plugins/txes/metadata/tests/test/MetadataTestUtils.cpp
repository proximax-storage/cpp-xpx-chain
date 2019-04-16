/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MetadataTestUtils.h"

namespace catapult { namespace test {

    std::unique_ptr<model::MetadataModification> CreateModification(
            model::MetadataModificationType type, uint8_t keySize, uint16_t valueSize) {
        uint32_t entitySize = sizeof(model::MetadataModification) + keySize + valueSize;
        auto pModification = utils::MakeUniqueWithSize<model::MetadataModification>(entitySize);
        pModification->Size = entitySize;
        pModification->ModificationType = type;
        pModification->KeySize = keySize;
        pModification->ValueSize = valueSize;

        return pModification;
    }
}}
