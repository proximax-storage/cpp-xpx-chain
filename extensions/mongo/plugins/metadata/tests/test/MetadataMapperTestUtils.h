/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "plugins/txes/metadata/src/state/MetadataEntry.h"
#include <bsoncxx/json.hpp>

namespace catapult { namespace test {

        /// Verifies that db metadata (\a dbMetadata) is equivalent to model metadata \a entry.
        void AssertEqualMetadataData(const state::MetadataEntry& entry, const bsoncxx::document::view& dbMetadata);
    }}
