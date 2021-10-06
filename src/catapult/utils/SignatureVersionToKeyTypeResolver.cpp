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
#include <catapult/types.h>
namespace catapult { namespace utils {

	/// Resolve an account version to the corresponding key hashing algorythm from a version
	KeyHashingType ResolveKeyHashingTypeFromSignatureVersion(SignatureVersion version)
	{
		if(version >= 2) return KeyHashingType::Sha2;
		return KeyHashingType::Sha3;
	}

	bool VerifyAccountVersionCompatibilityWithSignatureVersion(uint32_t accountVersion, SignatureVersion signatureVersion)
	{
		if(accountVersion == 2 && signatureVersion < 2) return false;
		if(accountVersion == 1 && signatureVersion > 1) return false;
		return true;
	}


}}
