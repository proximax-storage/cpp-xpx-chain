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
#include "Cosignature.h"
#include "Transaction.h"
#include <vector>
#include <plugins/txes/aggregate/src/model/AggregateTransaction.h>
#include "catapult/utils/AccountVersionFeatureResolver.h"

namespace catapult { namespace model {


	///Weak reference wrapper around a cosignature pointer
	struct WeakCosignatureContainer {

		///Notice that nullptr is not safely handled
		WeakCosignatureContainer() : m_Ptr(nullptr), m_Type(SignatureLayout::Raw), m_Size(0){

		}
		template<typename TCosignatureType>
		WeakCosignatureContainer(const std::vector<TCosignatureType>* cosignatures) : m_Ptr(cosignatures), m_Size(cosignatures->size())
		{
			if constexpr(std::is_same_v<TCosignatureType, model::AggregateTransactionRawDescriptor::CosignatureType>)
			{
				m_Type = SignatureLayout::Raw;
			}
			else m_Type = SignatureLayout::Extended;
		}

		bool hasCosigner(const Key& signer) const {
			//if(m_pTransaction->Signer == signer)
			//	return true;
			switch(m_Type)
			{
			case SignatureLayout::Extended:
			{
				auto* cosignatures = static_cast<const std::vector<Cosignature<SignatureLayout::Extended>>*>(m_Ptr);
				return std::any_of(cosignatures->cbegin(), cosignatures->cend(), [&signer](const auto& cosignature) {
				  return signer == cosignature.Signer;
				});
			}

			default:
			{
				auto* cosignatures = static_cast<const std::vector<Cosignature<SignatureLayout::Raw>>*>(m_Ptr);
				return std::any_of(cosignatures->cbegin(), cosignatures->cend(), [&signer](const auto& cosignature) {
				  return signer == cosignature.Signer;
				});
			}
			}
		}
		const size_t size(){
			return m_Size;
		}
		const size_t size() const{
			return m_Size;
		}
		std::vector<CosignatureInfo> AsInfo() const
		{
			switch(m_Type)
			{
			case SignatureLayout::Extended:
			{
				return *static_cast<const std::vector<Cosignature<SignatureLayout::Extended>>*>(m_Ptr);
			}

			default:
			{
				std::vector<CosignatureInfo> result;
				for(auto& cosignature : *static_cast<const std::vector<Cosignature<SignatureLayout::Raw>>*>(m_Ptr))
				{
					result.emplace_back(cosignature.Signer, crypto::SignatureFeatureSolver::ExpandSignature(cosignature.GetRawSignature(), cosignature.GetDerivationScheme()));
				}
				return result;
			}
			}
		}
		template<typename TCosignatureType>
		const std::vector<TCosignatureType>* Get() const {
			return static_cast<const std::vector<TCosignatureType>*>(m_Ptr);
		}

	private:
		//PS: Consider removing, only used in tests
		const size_t m_Size;
		SignatureLayout::SignatureLayout m_Type;
		const void* m_Ptr;

	};

	/// Wrapper around a transaction and its cosignatures.
	class WeakCosignedTransactionInfo {
	public:
		/// Creates an empty weak transaction info.
		WeakCosignedTransactionInfo() : WeakCosignedTransactionInfo(nullptr, nullptr)
		{}

		/// Creates a weak transaction info around \a pTransaction and \a pCosignatures.
		WeakCosignedTransactionInfo(const Transaction* pTransaction, const std::vector<model::CosignatureInfo>* pCosignatures)
				: m_pTransaction(pTransaction)
				, m_pCosignatures(pCosignatures)
		{}

	public:
		/// Gets the transaction.
		const Transaction& transaction() const {
			return *m_pTransaction;
		}

		const DerivationScheme tryGetDerivationSchemeForSigner(const Key& signer);
		/// Gets the cosignatures.
		const std::vector<model::CosignatureInfo>& cosignatures() const {
			return *m_pCosignatures;
		}

		/// Returns \c true if a cosignature from \a signer is present.
		bool hasCosigner(const Key& signer) const {
			return std::any_of(m_pCosignatures->cbegin(), m_pCosignatures->cend(), [&signer](const auto& cosignature) {
			  return signer == cosignature.Signer;
			});
		}

	public:
		/// Returns \c true if the info is non-empty and contains a valid entity pointer, \c false otherwise.
		explicit operator bool() const noexcept {
			return !!m_pTransaction;
		}

	private:
		const Transaction* m_pTransaction;
		const std::vector<model::CosignatureInfo>* m_pCosignatures;
	};
/*
	template<>
	const DerivationScheme WeakCosignedTransactionInfo<model::AggregateTransactionRawDescriptor>::tryGetDerivationSchemeForSigner(const Key& signer)
	{
		return DerivationScheme::Ed25519_Sha3;
	}

	template<>
	const DerivationScheme WeakCosignedTransactionInfo<model::AggregateTransactionExtendedDescriptor>::tryGetDerivationSchemeForSigner(const Key& signer)
	{
		for(auto& cosignature : *m_pCosignatures)
		{
			if(cosignature.Signer == signer)
				return cosignature.GetDerivationScheme();
		}
		return DerivationScheme::Unset;
	}
*/
}}
