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

#include "SecretLockInfoSerializer.h"

namespace catapult { namespace state {

	void SecretLockInfoExtendedDataSerializer::Save(const SecretLockInfo& lockInfo, io::OutputStream& output) {
		// write version
		io::Write32(output, 1);

		io::Write8(output, utils::to_underlying_type(lockInfo.HashAlgorithm));
		output.write(lockInfo.Secret);
		output.write(lockInfo.Recipient);
	}

	void SecretLockInfoExtendedDataSerializer::Load(io::InputStream& input, SecretLockInfo& lockInfo) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of SecretLockInfo", version);

		lockInfo.HashAlgorithm = static_cast<model::LockHashAlgorithm>(io::Read8(input));
		input.read(lockInfo.Secret);
		input.read(lockInfo.Recipient);
		lockInfo.CompositeHash = model::CalculateSecretLockInfoHash(lockInfo.Secret, lockInfo.Recipient);
	}
}}
