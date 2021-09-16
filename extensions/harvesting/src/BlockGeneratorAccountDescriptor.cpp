/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
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

#include "BlockGeneratorAccountDescriptor.h"

namespace catapult { namespace harvesting {

	namespace {
		crypto::KeyPair CreateZeroKeyPair() {
			return crypto::KeyPair::FromPrivate(crypto::PrivateKey(), 1);
		}
	}

	// Note: Empty descriptors always use V1 accounts
	BlockGeneratorAccountDescriptor::BlockGeneratorAccountDescriptor()
			: BlockGeneratorAccountDescriptor(CreateZeroKeyPair(), CreateZeroKeyPair(), 1)
	{}

	BlockGeneratorAccountDescriptor::BlockGeneratorAccountDescriptor(crypto::KeyPair&& signingKeyPair, crypto::KeyPair&& vrfKeyPair, uint32_t accountVersion)
			: m_signingKeyPair(std::move(signingKeyPair))
			, m_vrfKeyPair(std::move(vrfKeyPair))
			, m_accountVersion(accountVersion)
	{}

	const crypto::KeyPair& BlockGeneratorAccountDescriptor::signingKeyPair() const {
		return m_signingKeyPair;
	}

	const crypto::KeyPair& BlockGeneratorAccountDescriptor::vrfKeyPair() const {
		return m_vrfKeyPair;
	}

	const uint32_t BlockGeneratorAccountDescriptor::accountVersion() const {
		return m_accountVersion;
	}

	// V1 account comparisons ignore the VRF Key Pair as it is not used
	bool BlockGeneratorAccountDescriptor::operator==(const BlockGeneratorAccountDescriptor& rhs) const {
		return m_signingKeyPair.publicKey() == rhs.m_signingKeyPair.publicKey() &&
				( m_accountVersion < 2 || m_vrfKeyPair.publicKey() == rhs.m_vrfKeyPair.publicKey());
	}

	bool BlockGeneratorAccountDescriptor::operator!=(const BlockGeneratorAccountDescriptor& rhs) const {
		return !(*this == rhs);
	}
}}
