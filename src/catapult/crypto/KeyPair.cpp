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

#include "KeyPair.h"
#include "catapult/utils/HexParser.h"

namespace catapult { namespace crypto {

	template<class T>
	KeyPair KeyPair::FromString(const std::string& privateKey) {
		return FromPrivate<T>(PrivateKey::FromString(privateKey));
	}
	template<class T>
	KeyPair KeyPair::FromPrivate(PrivateKey&& privateKey) {
		return static_cast<KeyPair>(T(std::move(privateKey)));
	}

	KeyPair KeyPair::FromPrivate(PrivateKey&& privateKey, KeyHashingType hashingType) {
		if(hashingType == KeyHashingType::Sha2)
			return FromPrivate<KeyPairSha2>(std::move(privateKey));
		return FromPrivate<KeyPairSha3>(std::move(privateKey));

	}

	KeyPair KeyPair::FromString(const std::string& privateKey, KeyHashingType hashingType) {
		if(hashingType== KeyHashingType::Sha2)
			return FromString<KeyPairSha2>(privateKey);
		return FromString<KeyPairSha3>(privateKey);
	}
	KeyPair KeyPair::FromPrivate(PrivateKey&& privateKey, uint32_t version) {
		return FromPrivate(std::move(privateKey), utils::ResolveKeyHashingTypeFromAccountVersion(version));

	}
	KeyPair KeyPair::FromString(const std::string& privateKey, uint32_t version) {
		return FromString(privateKey, utils::ResolveKeyHashingTypeFromAccountVersion(version));
	}

}}
