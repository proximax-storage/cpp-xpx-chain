/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Receipt.h"
#include "ReceiptSource.h"
#include "EntityPtr.h"
#include "catapult/crypto/Hashes.h"
#include "catapult/utils/MemoryUtils.h"

namespace catapult { namespace model {

	template<ReceiptType StatementReceiptType>
	class Statement {
	public:
		/// Creates a statement around \a source.
		explicit Statement(const ReceiptSource& source) : m_source(source)
		{}

	public:
		/// Gets statement source.
		const ReceiptSource& source() const {
			return m_source;
		}

		/// Gets the number of attached receipts.
		size_t size() const {
			return m_receipts.size();
		}

		/// Gets the receipt at \a index.
		const Receipt& receiptAt(size_t index) const {
			return *m_receipts[index];
		}

		/// Calculates a unique hash for this statement.
		Hash256 hash() const {
			// prepend receipt header to statement
			auto version = static_cast<uint32_t>(1);
			auto type = StatementReceiptType;

			crypto::Sha3_256_Builder hashBuilder;
			hashBuilder.update({ reinterpret_cast<const uint8_t*>(&version), sizeof(uint32_t) });
			hashBuilder.update({ reinterpret_cast<const uint8_t*>(&type), sizeof(ReceiptType) });
			hashBuilder.update({ reinterpret_cast<const uint8_t*>(&m_source), sizeof(ReceiptSource) });

			auto receiptHeaderSize = sizeof(Receipt::Size);
			for (const auto& pReceipt : m_receipts) {
				hashBuilder.update({
				   reinterpret_cast<const uint8_t*>(pReceipt.get()) + receiptHeaderSize,
				   pReceipt->Size - receiptHeaderSize
				});
			}

			Hash256 hash;
			hashBuilder.final(hash);
			return hash;
		}

	public:
		/// Adds \a receipt to this transaction statement.
		void addReceipt(const Receipt& receipt) {
			// make a copy of the receipt
			auto pReceiptCopy = utils::MakeUniqueWithSize<Receipt>(receipt.Size);
			std::memcpy(static_cast<void*>(pReceiptCopy.get()), &receipt, receipt.Size);

			// insertion sort by receipt type
			auto iter = std::find_if(m_receipts.cbegin(), m_receipts.cend(), [&receipt](const auto& pReceipt) {
				return pReceipt->Type > receipt.Type;
			});

			m_receipts.insert(iter, std::move(pReceiptCopy));
		}

	private:
		ReceiptSource m_source;
		std::vector<UniqueEntityPtr<Receipt>> m_receipts;
	};

	/// Collection of receipts scoped to a transaction.
	using TransactionStatement = Statement<Receipt_Type_Transaction_Group>;

	/// Collection of receipts scoped to a public key.
	using PublicKeyStatement = Statement<Receipt_Type_Public_Key_Group>;
}}
