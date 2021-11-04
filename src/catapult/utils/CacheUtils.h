/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include "catapult/exceptions.h"
#include "catapult/state/AccountState.h"
#include "catapult/cache_core/ReadOnlyAccountStateCache.h"
#include "catapult/types.h"
#include "catapult/model/Address.h"
#include <optional>

namespace catapult { namespace utils {
		std::optional<std::reference_wrapper<const state::AccountState>> FindAccountStateByPublicKeyOrAddress(const cache::ReadOnlyAccountStateCache& cache, const Key& publicKey) {
			auto accountStateKeyIter = cache.find(publicKey);
			if (accountStateKeyIter.tryGet())
				return std::optional<std::reference_wrapper<const state::AccountState>>(accountStateKeyIter.get());

			// if state could not be accessed by public key, try searching by address
			auto accountStateAddressIter = cache.find(model::PublicKeyToAddress(publicKey, cache.networkIdentifier()));
			if (accountStateAddressIter.tryGet())
				return std::optional<std::reference_wrapper<const state::AccountState>>(accountStateAddressIter.get());

			return std::nullopt;
		}

        std::optional<std::reference_wrapper<const state::AccountState>> FindAccountStateByPublicKeyOrAddress(const cache::AccountStateCacheDelta& cache, const Key& publicKey) {
            auto accountStateKeyIter = cache.find(publicKey);
            if (accountStateKeyIter.tryGet())
                return std::optional<std::reference_wrapper<const state::AccountState>>(accountStateKeyIter.get());

            // if state could not be accessed by public key, try searching by address
            auto accountStateAddressIter = cache.find(model::PublicKeyToAddress(publicKey, cache.networkIdentifier()));
            if (accountStateAddressIter.tryGet())
                return std::optional<std::reference_wrapper<const state::AccountState>>(accountStateAddressIter.get());

            return std::nullopt;
        }
}}

