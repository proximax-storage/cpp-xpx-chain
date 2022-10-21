/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "plugins/txes/exchange_sda/src/state/SdaExchangeEntry.h"
#include "plugins/txes/exchange_sda/src/state/SdaOfferGroupEntry.h"
#include <bsoncxx/json.hpp>

namespace catapult { namespace test {

    /// Verifies that db SDA-SDA offer entry (\a dbExchangeEntry) is equivalent to model offer \a entry and \a address.
    void AssertEqualSdaExchangeData(const state::SdaExchangeEntry& entry, const Address& address, const bsoncxx::document::view& dbExchangeEntry);

    /// Verifies that db SDA-SDA offer group entry (\a dbExchangeEntry) is equivalent to model offer \a entry.
    void AssertEqualSdaOfferGroupData(const state::SdaOfferGroupEntry& entry, const bsoncxx::document::view& dbExchangeEntry);
}}