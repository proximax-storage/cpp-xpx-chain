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
#include "catapult/model/EntityPtr.h"
#include "catapult/types.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a receipt entity.
	struct Receipt : public SizePrefixedEntity {
		/// Receipt version.
        VersionType Version;

		/// Receipt type.
		ReceiptType Type;
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
	struct DriveStateReceipt : public Receipt {
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

	/// Binary layout for a offer creation receipt.
	struct OfferCreationReceipt : public Receipt {
	public:
		/// Creates a receipt around \a receiptType, \a sender, \a mosaicsPair, \a amountGive and \a amountGet.
		OfferCreationReceipt(
				ReceiptType receiptType,
				const Key& sender,
				std::pair<MosaicId, MosaicId> mosaicsPair,
				catapult::Amount amountGive,
				catapult::Amount amountGet)
				: Sender(sender)
				, MosaicsPair(mosaicsPair)
				, AmountGive(amountGive)
				, AmountGet(amountGet) {
			Size = sizeof(OfferCreationReceipt);
			Version = 1;
			Type = receiptType;
		}

	public:
		/// Mosaic sender public key.
		Key Sender;

		/// Mosaics pair of mosaic id to give and to get.
		std::pair<MosaicId, MosaicId> MosaicsPair;

		/// Amount to give.
		catapult::Amount AmountGive;

		/// Amount to get.
		catapult::Amount AmountGet;
	};

	/// Binary layout for a offer exchange receipt.
	struct ExchangeDetail{
		/// Mosaic recipient address.
		Address Recipient;

		/// Mosaics pair of mosaic id to give and to get of recipient.
		std::pair<MosaicId, MosaicId> MosaicsPair;

		/// Amount given by recipient.
		catapult::Amount AmountGive;

		/// Amount gotten by recipient.
		catapult::Amount AmountGet;
	};

	struct OfferExchangeReceipt : public Receipt {
	public:
		/// Mosaic sender public key.
		Key Sender;

		/// Mosaics pair of mosaic id to give and to get of sender.
		std::pair<MosaicId, MosaicId> MosaicsPair;

		/// Count of exchange details.
		uint16_t ExchangeDetailCount;
	};

	UniqueEntityPtr<OfferExchangeReceipt> CreateOfferExchangeReceipt(
			ReceiptType receiptType,
			const Key& sender,
			const std::pair<MosaicId, MosaicId>& mosaicsPair,
			const std::vector<ExchangeDetail>& exchangeDetails);

	/// Binary layout for a offer removal receipt.
	struct OfferRemovalReceipt : public Receipt {
	public:
		/// Creates a receipt around \a receiptType, \a sender, \a mosaicsPair and \a amountGiveReturned.
		OfferRemovalReceipt(
				ReceiptType receiptType,
				const Key& sender,
				std::pair<MosaicId, MosaicId> mosaicsPair,
				catapult::Amount amountGiveReturned)
				: Sender(sender)
				, MosaicsPair(mosaicsPair)
				, AmountGiveReturned(amountGiveReturned) {
			Size = sizeof(OfferRemovalReceipt);
			Version = 1;
			Type = receiptType;
		}

	public:
		/// Mosaic sender public key.
		Key Sender;

		/// Mosaics pair of mosaic id to give and to get.
		std::pair<MosaicId, MosaicId> MosaicsPair;

		/// Amount to give that has been returned to sender.
		catapult::Amount AmountGiveReturned;
	};

	/// Binary layout for a storage receipt.
	struct StorageReceipt : public Receipt {
	public:
		/// Creates a receipt around \a receiptType, \a sender, \a recipient, \a mosaicsPair and \a sentAmount.
		StorageReceipt(
				ReceiptType receiptType,
				const Key& sender,
				const Key& recipient,
				std::pair<MosaicId, MosaicId> mosaicsPair,
				catapult::Amount sentAmount)
				: Sender(sender)
				, Recipient(recipient)
				, MosaicsPair(mosaicsPair)
				, SentAmount(sentAmount) {
			Size = sizeof(StorageReceipt);
			Version = 1;
			Type = receiptType;
		}

	public:
		/// Sender public key.
		Key Sender;

		/// Recipient public key.
		Key Recipient;

		/// Mosaic IDs which are being sent by Sender and received by Recipient respectively.
		std::pair<MosaicId, MosaicId> MosaicsPair;

		/// Amount of the mosaic that is being sent.
		catapult::Amount SentAmount;
	};

#pragma pack(pop)

/// Defines constants for a receipt with \a TYPE and \a VERSION.
#define DEFINE_RECEIPT_CONSTANTS(TYPE, VERSION) \
	/* Receipt format version. */ \
	static constexpr VersionType Current_Version = VERSION; \
	/* Receipt type. */ \
	static constexpr ReceiptType Receipt_Type = TYPE;
}}
