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
#include "AggregateEntityType.h"
#include "catapult/model/Cosignature.h"
#include "catapult/model/EntityType.h"
#include "catapult/model/Transaction.h"
#include "catapult/model/TransactionContainer.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for an aggregate transaction header.
	template<typename TDescriptor>
	struct AggregateTransactionHeader : Transaction {
	public:
		static constexpr VersionType Min_Version = TDescriptor::MinVersion;
		DEFINE_TRANSACTION_CONSTANTS(TDescriptor::CompleteType, TDescriptor::Version)

	public:
		/// Transaction payload size in bytes.
		/// \note This is the total number bytes occupied by all sub-transactions.
		uint32_t PayloadSize;

		// followed by sub-transaction data
		// followed by cosignatures data

	public:
		size_t GetHeaderSize() const {
			return sizeof(AggregateTransactionHeader);
		}
	};


	/// Binary layout for an aggregate transaction.

	namespace detail {
		template<typename TDescriptor>
		struct AggregateTransactionImpl : public TransactionContainer<AggregateTransactionHeader<TDescriptor>, EmbeddedTransaction> {
			using CosignatureType = typename TDescriptor::CosignatureType;
		private:
			template<typename T>
			static auto* CosignaturesPtrT(T& transaction) {
				return transaction.Size <= sizeof(T) + transaction.PayloadSize
					   ? nullptr
					   : transaction.ToBytePointer() + sizeof(T) + transaction.PayloadSize;
			}

			template<typename T>
			static size_t CosignaturesCountT(T& transaction) {
				return transaction.Size <= sizeof(T) + transaction.PayloadSize
					   ? 0
					   : (transaction.Size - sizeof(T) - transaction.PayloadSize) / sizeof(CosignatureType);
			}

		public:
			/// Returns a const pointer to the first cosignature contained in this transaction.
			/// \note The returned pointer is undefined if the aggregate has an invalid size.
			const CosignatureType* CosignaturesPtr() const {
				return reinterpret_cast<const CosignatureType*>(CosignaturesPtrT(*this));
			}

			/// Returns a pointer to the first cosignature contained in this transaction.
			/// \note The returned pointer is undefined if the aggregate has an invalid size.
			CosignatureType* CosignaturesPtr() {
				return reinterpret_cast<CosignatureType*>(CosignaturesPtrT(*this));
			}

			/// Returns the number of cosignatures attached to this transaction.
			/// \note The returned value is undefined if the aggregate has an invalid size.
			size_t CosignaturesCount() const {
				return CosignaturesCountT(*this);
			}

			/// Returns the number of cosignatures attached to this transaction.
			/// \note The returned value is undefined if the aggregate has an invalid size.
			size_t CosignaturesCount() {
				return CosignaturesCountT(*this);
			}
		};
	}


	template<typename TDescriptor>
	struct AggregateTransaction : detail::AggregateTransactionImpl<TDescriptor> {};


	struct AggregateTransactionRawDescriptor
	{
		static constexpr VersionType MinVersion = 2;
		static constexpr VersionType Version = 3;
		static constexpr model::EntityType BondedType = model::Entity_Type_Aggregate_Bonded_V1;
		static constexpr model::EntityType CompleteType = model::Entity_Type_Aggregate_Complete_V1;
		static constexpr SignatureLayout::SignatureLayout CosignatureLayout = SignatureLayout::Raw;
		using CosignatureType = Cosignature<CosignatureLayout>;
	};

	struct AggregateTransactionExtendedDescriptor
	{
		static constexpr VersionType MinVersion = 1;
		static constexpr VersionType Version = 1;
		static constexpr model::EntityType BondedType = model::Entity_Type_Aggregate_Bonded_V2;
		static constexpr model::EntityType CompleteType = model::Entity_Type_Aggregate_Complete_V2;
		static constexpr SignatureLayout::SignatureLayout CosignatureLayout = SignatureLayout::Extended;
		using CosignatureType = Cosignature<CosignatureLayout>;
	};
#pragma pack(pop)


	/// Gets the number of bytes containing transaction data according to \a header.
	template<typename TDescriptor>
	size_t GetTransactionPayloadSize(const AggregateTransactionHeader<TDescriptor>& header);

	/// Checks the real size of \a aggregate against its reported size and returns \c true if the sizes match.
	/// \a registry contains all known transaction types.
	template<typename TDescriptor>
	bool IsSizeValid(const AggregateTransaction<TDescriptor>& aggregate, const TransactionRegistry& registry);
}}
