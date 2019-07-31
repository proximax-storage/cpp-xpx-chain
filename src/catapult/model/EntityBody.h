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
#include "EntityType.h"
#include "NetworkInfo.h"
#include "catapult/utils/Casting.h"
#include "catapult/types.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

    namespace {
        constexpr VersionType ENTITY_VERSION_MASK = VersionType(~VersionType{0}) >> (sizeof(NetworkIdentifier) * CHAR_BIT);
        constexpr uint8_t NETWORK_IDENTIFIER_SHIFT = (sizeof(VersionType) - sizeof(NetworkIdentifier)) * CHAR_BIT;
    }

	/// Binary layout for an entity body.
	template<typename THeader>
	struct EntityBody : public THeader {
	public:
		/// Entity signer's public key.
		Key Signer;

		/// Entity version.
        VersionType Version;

		/// Entity type.
		EntityType Type;

		/// Returns network of an entity, as defined in NetworkInfoTraits.
		NetworkIdentifier Network() const {
			return static_cast<NetworkIdentifier>(Version >> NETWORK_IDENTIFIER_SHIFT);
		}

		/// Returns version of an entity.
        VersionType EntityVersion() const {
			return static_cast<VersionType>(Version & ENTITY_VERSION_MASK);
		}
	};

#pragma pack(pop)

	/// Creates a version field out of given entity \a version and \a networkIdentifier.
	constexpr VersionType MakeVersion(NetworkIdentifier networkIdentifier, VersionType version) noexcept {
		return static_cast<VersionType>(utils::to_underlying_type(networkIdentifier)) << NETWORK_IDENTIFIER_SHIFT | version;
	}
}}
