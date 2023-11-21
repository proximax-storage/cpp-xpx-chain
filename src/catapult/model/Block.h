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
#include "Transaction.h"
#include "TransactionContainer.h"
#include "catapult/model/Cosignature.h"
#include "catapult/types.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a block header.
	struct BlockHeader : public VerifiableEntity {
	public:
		/// Block format version.
		static constexpr VersionType Current_Version = 4;

	public:
		/// Height of a block.
		catapult::Height Height;

		/// Timestamp of a block.
		catapult::Timestamp Timestamp;

		/// Difficulty of a block.
		catapult::Difficulty Difficulty;

		/// Fee multiplier applied to transactions contained in block.
		BlockFeeMultiplier FeeMultiplier;

		/// Hash of the previous block.
		Hash256 PreviousBlockHash;

		/// Aggregate hash of a block's transactions.
		Hash256 BlockTransactionsHash;

		/// Aggregate hash of a block's receipts.
		Hash256 BlockReceiptsHash;

		/// Hash of the global chain state at this block.
		Hash256 StateHash;

		/// Public key of optional beneficiary designated by harvester.
		Key Beneficiary;

		/// The part of the transaction fee harvester is willing to get.
		/// From 0 up to FeeInterestDenominator. The customer gets
		/// (FeeInterest / FeeInterestDenominator)'th part of the maximum transaction fee.
		uint32_t FeeInterest;

		/// Denominator of the transaction fee.
		uint32_t FeeInterestDenominator;

		// followed by transaction data

	public:
		size_t GetHeaderSize() const;
	};

	/// Binary layout for a block header v4.
	struct BlockHeaderV4 : public BlockHeader {
	public:
		/// Committee round (number of attempts to generate this block).
		int64_t Round;

		/// Time lapse in milliseconds of the committee phase of the round this block was generated in.
		uint64_t CommitteePhaseTime;

		/// Transaction payload size in bytes.
		/// \note This is the total number of bytes occupied by all transactions.
		uint32_t TransactionPayloadSize;

		// followed by transaction data
		// followed by cosignatures data
	};

	/// Binary layout for a block.
	struct Block : public TransactionContainer<BlockHeader, Transaction> {
	private:
		static auto* CosignaturesPtrT(Block& block) {
			return (block.EntityVersion() < 4 || block.Size <= sizeof(BlockHeaderV4) + reinterpret_cast<const BlockHeaderV4&>(block).TransactionPayloadSize)
				? nullptr
				: block.ToBytePointer() + sizeof(BlockHeaderV4) + reinterpret_cast<const BlockHeaderV4&>(block).TransactionPayloadSize;
		}

		static size_t CosignaturesCountT(Block& block) {
			return (block.EntityVersion() < 4 || block.Size <= sizeof(BlockHeaderV4) + reinterpret_cast<const BlockHeaderV4&>(block).TransactionPayloadSize)
				? 0
				: (block.Size - sizeof(BlockHeaderV4) - reinterpret_cast<const BlockHeaderV4&>(block).TransactionPayloadSize) / sizeof(Cosignature);
		}

	public:
		/// Returns a const pointer to the first cosignature contained in this block.
		/// \note The returned pointer is null if the block has an invalid size.
		const Cosignature* CosignaturesPtr() const {
			return reinterpret_cast<const Cosignature*>(CosignaturesPtrT(const_cast<Block&>(*this)));
		}

		/// Returns a pointer to the first cosignature contained in this block.
		/// \note The returned pointer is null if the block has an invalid size.
		Cosignature* CosignaturesPtr() {
			return reinterpret_cast<Cosignature*>(CosignaturesPtrT(*this));
		}

		/// Returns the number of cosignatures attached to this block.
		/// \note The returned value is zero if the block has an invalid size.
		size_t CosignaturesCount() const {
			return CosignaturesCountT(const_cast<Block&>(*this));
		}

		/// Returns the number of cosignatures attached to this block.
		/// \note The returned value is zero if the block has an invalid size.
		size_t CosignaturesCount() {
			return CosignaturesCountT(*this);
		}

		int64_t round() const {
			switch (EntityVersion()) {
				case 3:
					return 0u;
				default:
					return reinterpret_cast<const BlockHeaderV4&>(*this).Round;
			}
		}

		void setRound(int64_t value) {
			if (EntityVersion() > 3)
				reinterpret_cast<BlockHeaderV4&>(*this).Round = value;
		}

		uint64_t committeePhaseTime() const {
			switch (EntityVersion()) {
				case 3:
					return 0u;
				default:
					return reinterpret_cast<const BlockHeaderV4&>(*this).CommitteePhaseTime;
			}
		}

		void setCommitteePhaseTime(uint64_t value) {
			if (EntityVersion() > 3)
				reinterpret_cast<BlockHeaderV4&>(*this).CommitteePhaseTime = value;
		}

		uint32_t transactionPayloadSize() const {
			switch (EntityVersion()) {
				case 3:
					return 0u;
				default:
					return reinterpret_cast<const BlockHeaderV4&>(*this).TransactionPayloadSize;
			}
		}

		void setTransactionPayloadSize(uint32_t value) {
			if (EntityVersion() > 3)
				reinterpret_cast<BlockHeaderV4&>(*this).TransactionPayloadSize = value;
		}
	};

#pragma pack(pop)

	/// Gets the number of bytes containing transaction data according to \a header.
	size_t GetTransactionPayloadSize(const BlockHeader& header);

	/// Checks the real size of \a block against its reported size and returns \c true if the sizes match.
	/// \a registry contains all known transaction types.
	bool IsSizeValid(const Block& block, const TransactionRegistry& registry);
}}
