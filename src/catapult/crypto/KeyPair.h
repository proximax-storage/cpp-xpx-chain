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
#include "catapult/utils/AccountVersionFeatureResolver.h"

namespace catapult { namespace crypto {

#ifdef SPAMMER_TOOL
#pragma pack(push, 16)
#endif

	/// Represents an interface to a pair of private key with associated public key. Read Only
	class KeyPair {

	protected:
		KeyPair(PrivateKey&& privateKey) : m_privateKey(std::move(privateKey)) {}
	public:
		/// Creates a key pair of given type from \a privateKey.
		template<class T>
		static KeyPair FromPrivate(PrivateKey&& privateKey);

		/// Creates a key pair of a given type from \a privateKey string.
		template<class T>
		static KeyPair FromString(const std::string& privateKey);
		/// Creates a key pair of given type from \a privateKey based on account version.
		static KeyPair FromPrivate(PrivateKey&& privateKey, DerivationScheme hashingType);

		/// Creates a key pair of a given type from \a privateKey string.
		static KeyPair FromString(const std::string& privateKey, DerivationScheme hashingType);

		/// Creates a key pair of given type from \a privateKey based on account version.
		static KeyPair FromPrivate(PrivateKey&& privateKey, uint32_t version);

		/// Creates a key pair of a given type from \a privateKey string.
		static KeyPair FromString(const std::string& privateKey, uint32_t version);

		/// Returns a private key of a key pair.
		const auto& privateKey() const {
			return m_privateKey;
		}

		/// Returns the hashing algorythm used in this KeyPair.
		const auto derivationScheme() const {
			return m_derivationScheme;
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
		DerivationScheme m_derivationScheme;
	};

	/// Represents a pair of private key with associated public key associated with a Sha3 hashing function
	class KeyPairSha3 final: public KeyPair {
		friend KeyPair;

	protected:
		KeyPairSha3(PrivateKey&& privateKey) : KeyPair(std::move(privateKey)) {
			DerivePublicKeyFromPrivate(m_privateKey);
			m_derivationScheme = DerivationScheme::Ed25519_Sha3;
		}

	protected:
		void DerivePublicKeyFromPrivate(const PrivateKey& key) {
			ExtractPublicKeyFromPrivateKey<DerivationScheme::Ed25519_Sha3>(key, m_publicKey);
		}
	};

	/// Represents a pair of private key with associated public key associated with a Sha3 hashing function
	class KeyPairSha2 final: public KeyPair {
		friend KeyPair;
	protected:
		KeyPairSha2(PrivateKey&& privateKey) : KeyPair(std::move(privateKey)) {
			DerivePublicKeyFromPrivate(m_privateKey);
			m_derivationScheme = DerivationScheme::Ed25519_Sha2;
		}

	protected:
		void DerivePublicKeyFromPrivate(const PrivateKey& key) {
			ExtractPublicKeyFromPrivateKey<DerivationScheme::Ed25519_Sha2>(key, m_publicKey);
		}
	};

#ifdef SPAMMER_TOOL
#pragma pack(pop)
#endif
}}
