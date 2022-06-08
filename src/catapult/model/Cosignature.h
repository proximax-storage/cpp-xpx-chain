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
#include "catapult/crypto/Signature.h"
#include "catapult/types.h"

namespace catapult { namespace model {

#pragma pack(push, 1)



	/// A cosignature.
	template<SignatureLayout::SignatureLayout TSignatureLayout>
	struct Cosignature;

	using CosignatureInfo = Cosignature<SignatureLayout::Extended>;
	struct CosignatureRawType {};
	struct CosignatureExtendedType {};

	template<>
	struct Cosignature<SignatureLayout::Extended> {

		using Tag = CosignatureExtendedType;
		Cosignature() = default;
		Cosignature(Key signer, catapult::Signature signature) : Signer(signer), Signature(signature)
		{
		}

		Cosignature(const CosignatureInfo& cosignature) : Signer(cosignature.Signer), Signature(cosignature.Signature)
		{
		}
		/// Cosigner public key.
		Key Signer;

		/// Cosigner signature.
		catapult::Signature Signature;

		DerivationScheme GetDerivationScheme() const
		{
			return crypto::SignatureFeatureSolver::GetDerivationScheme(Signature);
		}

		const catapult::RawSignature& GetRawSignature() const {
			return crypto::SignatureFeatureSolver::GetRawSignature(Signature);
		}

		CosignatureInfo ToInfo() const {
			return *this;
		}
	};


	template<>
	struct Cosignature<SignatureLayout::Raw> {

		using Tag = CosignatureRawType;
		Cosignature() = default;
		Cosignature(Key signer, catapult::RawSignature signature) : Signer(signer), Signature(signature)
		{
		}

		Cosignature(const CosignatureInfo& cosignature) : Signer(cosignature.Signer), Signature(cosignature.GetRawSignature())
		{
			if(cosignature.GetDerivationScheme() != DerivationScheme::Ed25519_Sha3 &&
				cosignature.GetDerivationScheme() != DerivationScheme::Unset)
				CATAPULT_THROW_INVALID_ARGUMENT("Raw cosignature layout does not support this derivation scheme.");
		}
		/// Cosigner public key.
		Key Signer;

		/// Cosigner signature.
		catapult::RawSignature Signature;

		constexpr DerivationScheme GetDerivationScheme() const
		{
			return DerivationScheme::Ed25519_Sha3;
		}

		const catapult::RawSignature& GetRawSignature() const{
			return Signature;
		}

		CosignatureInfo ToInfo() const {
			return CosignatureInfo(Signer, catapult::Signature(crypto::SignatureFeatureSolver::ExpandSignature(GetRawSignature(), GetDerivationScheme())));
		}
	};

	/// A weak pointer to a cosignature. Remove and rework!!!
	struct WeakCosignaturePtr {

		WeakCosignaturePtr() : m_Type(SignatureLayout::Raw) {}

		template<typename TCosignatureType>
		WeakCosignaturePtr(const TCosignatureType* cosignature) : m_Ptr(cosignature)
		{
			if constexpr(std::is_same_v<TCosignatureType, model::CosignatureInfo>)
			{
				m_Type = SignatureLayout::Extended;
			}
			else m_Type = SignatureLayout::Raw;
		}
		WeakCosignaturePtr operator++(int)
		{
			if(m_Type == SignatureLayout::Extended)
			{
				WeakCosignaturePtr temp = *this;
				m_Ptr = static_cast<const Cosignature<SignatureLayout::Extended>*>(m_Ptr)+1;
				return temp;

			}
			WeakCosignaturePtr temp = *this;
			m_Ptr = static_cast<const Cosignature<SignatureLayout::Raw>*>(m_Ptr)+1;
			return temp;
		}
		operator bool() const
		{
			return m_Ptr != nullptr;
		}

		WeakCosignaturePtr& operator++()
		{
			if(m_Type == SignatureLayout::Extended)
			{
				m_Ptr = static_cast<const Cosignature<SignatureLayout::Extended>*>(m_Ptr)+1;
				return *this;
			}
			m_Ptr = static_cast<const Cosignature<SignatureLayout::Raw>*>(m_Ptr)+1;
			return *this;
		}
		WeakCosignaturePtr operator[](int index) const
		{
			if(m_Type == SignatureLayout::Extended)
			{
				return WeakCosignaturePtr(static_cast<const Cosignature<SignatureLayout::Extended>*>(m_Ptr)+index);
			}
			return WeakCosignaturePtr(static_cast<const Cosignature<SignatureLayout::Raw>*>(m_Ptr)+index);
		}

		template<typename TCosignatureType>
		const TCosignatureType* Get() const
		{
			return static_cast<const TCosignatureType*>(m_Ptr);
		}

		const Key& Signer() const{
			//Signer is always first element
			return static_cast<const Cosignature<SignatureLayout::Raw>*>(m_Ptr)->Signer;
		}

		const RawSignature& GetRawSignature() const{
			if(m_Type == SignatureLayout::Extended)
			{
				return static_cast<const Cosignature<SignatureLayout::Extended>*>(m_Ptr)->GetRawSignature();
			}
			return static_cast<const Cosignature<SignatureLayout::Raw>*>(m_Ptr)->GetRawSignature();
		}

		const DerivationScheme GetDerivationScheme() const {
			if(m_Type == SignatureLayout::Extended)
			{
				return static_cast<const Cosignature<SignatureLayout::Extended>*>(m_Ptr)->GetDerivationScheme();
			}
			return DerivationScheme::Ed25519_Sha3;
		}
	private:
		const void* m_Ptr;
		SignatureLayout::SignatureLayout m_Type;
	};

	/// A detached cosignature.
	struct DetachedCosignature
	{

		/// Creates a detached cosignature around \a signer, \a raw signature and \a parentHash.
		DetachedCosignature(const Key& signer, const catapult::RawSignature& signature, const Hash256& parentHash)
				: Signer(signer)
				, Signature(signature)
				, Scheme(DerivationScheme::Ed25519_Sha3)
				, ParentHash(parentHash)
		{}

		/// Creates a detached cosignature around \a signer, \a signature and \a parentHash.
		DetachedCosignature(const Key& signer, const catapult::Signature& signature, const Hash256& parentHash)
				: Signer(signer)
				, Signature(crypto::SignatureFeatureSolver::GetRawSignature(signature))
				, Scheme(crypto::SignatureFeatureSolver::GetDerivationScheme(signature))
				, ParentHash(parentHash)
		{}

		/// Creates a detached cosignature around \a signer, \a signature and \a parentHash.
		DetachedCosignature(const Key& signer, const catapult::RawSignature& signature, DerivationScheme scheme, const Hash256& parentHash)
				: Signer(signer)
				, Signature(signature)
				, Scheme(scheme)
				, ParentHash(parentHash)
		{}

		/// Cosigner public key.
		Key Signer;

		/// Cosigner signature.
		catapult::RawSignature Signature;

		/// Signature derivation scheme
		DerivationScheme Scheme;

		/// Hash of the corresponding parent.
		Hash256 ParentHash;

		CosignatureInfo ToInfo() const {
			return {Signer, crypto::SignatureFeatureSolver::ExpandSignature(Signature, Scheme)};
		}
	};

#pragma pack(pop)
}}
