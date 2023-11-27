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
#include "ReceiptType.h"
#include "SizePrefixedEntity.h"
#include "catapult/types.h"
#include "StateChangeTracking.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a receipt entity.
	struct Receipt : public SizePrefixedEntity {
		/// Receipt version.
        VersionType Version;

		/// Receipt type.
		ReceiptType Type;
	};

	/// Binary layout for a public key receipt entity.
	/// IMPORTANT: First member of this receipt type must be the identifying public key!
	struct PublicKeyReceipt : public Receipt {

		Key GetAssociatedKey() const{
			return *reinterpret_cast<const Key*>(this);
		}
		template<typename T>
		static Key ExtractKey(const T* Data) {
			return reinterpret_cast<const PublicKeyReceipt*>(Data)->GetAssociatedKey();
		}
	};

	/// Binary layout for a balance transfer receipt.
	struct BalanceTransferReceipt : public Receipt {
	public:
		/// Creates a receipt around \a receiptType, \a sender, \a recipient, \a mosaicId and \a amount.
		BalanceTransferReceipt(
				ReceiptType receiptType,
				const Key& sender,
				const Address& recipient,
				catapult::MosaicId mosaicId,
				catapult::Amount amount)
				: Sender(sender)
				, Recipient(recipient)
				, MosaicId(mosaicId)
				, Amount(amount) {
			Size = sizeof(BalanceTransferReceipt);
			Version = 1;
			Type = receiptType;
		}

	public:
		/// Mosaic sender public key.
		Key Sender;

		/// Mosaic recipient address.
		Address Recipient;

		/// Mosaic id.
		catapult::MosaicId MosaicId;

		/// Amount.
		catapult::Amount Amount;
	};

	/// Binary layout for a global state change receipt
	struct GlobalStateChangeReceipt : public Receipt {
	public:
		/// Creates a receipt around \a flags.
		GlobalStateChangeReceipt(
				ReceiptType receiptType,
				model::StateChangeFlags flags)
			: Flags(flags){
			Size = sizeof(GlobalStateChangeReceipt);
			Version = 1;
			Type = receiptType;
		}

	public:
		/// Amount of mosaic.
		model::StateChangeFlags Flags;
	};

	/// Binary layout for a signer balance receipt to record the importance of the signer account
	struct SignerBalanceReceipt : public Receipt {
	public:
		/// Creates a receipt around \a amount and \a lockedAmount.
		SignerBalanceReceipt(
				ReceiptType receiptType,
				catapult::Amount amount,
				catapult::Amount lockedAmount)
				: Amount(amount)
				, LockedAmount(lockedAmount){
			Size = sizeof(SignerBalanceReceipt);
			Version = 1;
			Type = receiptType;
		}

	public:
		/// Amount of mosaic.
		catapult::Amount Amount;

		/// Amount of locked mosaic.
		catapult::Amount LockedAmount;
	};

	/// Binary layout for a balance change receipt.
	struct BalanceChangeReceipt : public Receipt {
	public:
		/// Creates a receipt around \a receiptType, \a account, \a mosaicId and \a amount.
		BalanceChangeReceipt(ReceiptType receiptType, const Key& account, catapult::MosaicId mosaicId, catapult::Amount amount)
				: Account(account)
				, MosaicId(mosaicId)
				, Amount(amount) {
			Size = sizeof(BalanceChangeReceipt);
			Version = 1;
			Type = receiptType;
		}

	public:
		/// Account public key.
		Key Account;

		/// Mosaic id.
		catapult::MosaicId MosaicId;

		/// Amount.
		catapult::Amount Amount;
	};

	/// Binary layout for an inflation receipt.
	struct InflationReceipt : public Receipt {
	public:
		/// Creates a receipt around \a receiptType, \a mosaicId and \a amount.
		InflationReceipt(ReceiptType receiptType, catapult::MosaicId mosaicId, catapult::Amount amount)
				: MosaicId(mosaicId)
				, Amount(amount) {
			Size = sizeof(InflationReceipt);
			Version = 1;
			Type = receiptType;
		}

	public:
		/// Mosaic id.
		catapult::MosaicId MosaicId;

		/// Amount.
		catapult::Amount Amount;
	};

	/// Binary layout for an artifact expiry receipt.
	template<typename TArtifactId>
	struct ArtifactExpiryReceipt : public Receipt {
	public:
		/// Creates a receipt around \a receiptType and \a artifactId.
		ArtifactExpiryReceipt(ReceiptType receiptType, TArtifactId artifactId) : ArtifactId(artifactId) {
			Size = sizeof(ArtifactExpiryReceipt);
			Version = 1;
			Type = receiptType;
		}

	public:
		/// Artifact id.
		TArtifactId ArtifactId;
	};

	/// Binary layout for a drive receipt.
	struct DriveStateReceipt : public PublicKeyReceipt {
	public:
		/// Creates a receipt around \a receiptType, \a sender, \a recipient, \a mosaicId and \a amount.
		DriveStateReceipt(
			ReceiptType receiptType,
			const Key& driveKey,
			uint8_t driveState)
			: DriveKey(driveKey)
			, DriveState(driveState) {
			Size = sizeof(DriveStateReceipt);
			Version = 1;
			Type = receiptType;
		}

	public:
		/// Drive public key.
		Key DriveKey;

		/// Drive state.
		uint8_t DriveState;
	};

#pragma pack(pop)

/// Defines constants for a receipt with \a TYPE and \a VERSION.
#define DEFINE_RECEIPT_CONSTANTS(TYPE, VERSION) \
	/* Receipt format version. */ \
	static constexpr VersionType Current_Version = VERSION; \
	/* Receipt type. */ \
	static constexpr ReceiptType Receipt_Type = TYPE;
}}
