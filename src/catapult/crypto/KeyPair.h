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
#include "KeyGenerator.h"
#include "PrivateKey.h"
#include "catapult/types.h"

namespace catapult { namespace crypto {

#ifdef SPAMMER_TOOL
#pragma pack(push, 16)
#endif

	/// Represents a pair of private key with associated public key.
	class KeyPair {
	protected:
		KeyPair(PrivateKey&& privateKey) : m_privateKey(std::move(privateKey)) {
			DerivePublicKeyFromPrivate(m_privateKey);
		}

	protected:
		virtual void DerivePublicKeyFromPrivate(const PrivateKey& key) //Remember to turn this into pure virtual method
		{
			ExtractPublicKeyFromPrivateKeySha3(key, m_publicKey);
		}
	public:
		/// Creates a key pair from \a privateKey.
		static auto FromPrivate(PrivateKey&& privateKey) {
			return KeyPair(std::move(privateKey));
		}

		/// Creates a key pair from \a privateKey.
		static auto FromString(const std::string& privateKey) {
			return FromPrivate(PrivateKey::FromString(privateKey));
		}

		/// Returns a private key of a key pair.
		const auto& privateKey() const {
			return m_privateKey;
		}
		/// Checks whether two keypairs are the same.
		bool operator==(const KeyPair& rhs) const { return this->m_privateKey == rhs.m_privateKey; }

		/// Checks whether two keypairs are not the same.
		bool operator!=(const KeyPair& rhs) const { return !operator==(rhs); }
		/// Returns a public key of a key pair.
		const auto& publicKey() const {
			return m_publicKey;
		}

	protected:
		PrivateKey m_privateKey;
		Key m_publicKey;
	};

	/// Represents a pair of private key with associated public key associated with a Sha3 hashing function
	class KeyPairSha3 final: public KeyPair {
	private:
		KeyPairSha3(PrivateKey&& privateKey) : KeyPair(std::move(privateKey)) {}

	protected:
		void DerivePublicKeyFromPrivate(const PrivateKey& key) override {
			ExtractPublicKeyFromPrivateKeySha3(key, m_publicKey);
		}
	};

	/// Represents a pair of private key with associated public key associated with a Sha3 hashing function
	class KeyPairSha2 final: public KeyPair {
	private:
		KeyPairSha2(PrivateKey&& privateKey) : KeyPair(std::move(privateKey)) {}

	protected:
		void DerivePublicKeyFromPrivate(const PrivateKey& key) override {
			ExtractPublicKeyFromPrivateKeySha2(key, m_publicKey);
		}
	};
#ifdef SPAMMER_TOOL
#pragma pack(pop)
#endif
}}
