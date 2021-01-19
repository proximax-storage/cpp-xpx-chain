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
#include "ContainerTypes.h"
#include "Cosignature.h"
#include "EntityType.h"
#include "NetworkInfo.h"
#include "NotificationType.h"
#include "catapult/utils/ArraySet.h"
#include "catapult/utils/ConfigurationBag.h"
#include "catapult/utils/TimeSpan.h"
#include "catapult/types.h"
#include <vector>

namespace catapult { namespace model {

	/// A basic notification.
	struct Notification {
	public:
		/// Creates a new notification with \a type and \a size.
		explicit Notification(NotificationType type, size_t size)
				: Type(type)
				, Size(size)
		{}

	public:
		/// Notification type.
		NotificationType Type;

		/// Notification size.
		size_t Size;
	};

	// region account

	/// Notification of use of an account address.
	template<VersionType version>
	struct AccountAddressNotification;

	template<>
	struct AccountAddressNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Core_Register_Account_Address_v1_Notification;

	public:
		/// Creates a notification around \a address.
		explicit AccountAddressNotification(const UnresolvedAddress& address)
				: Notification(Notification_Type, sizeof(AccountAddressNotification<1>))
				, Address(address)
		{}

	public:
		/// Address.
		UnresolvedAddress Address;
	};

	/// Notification of use of an account public key.
	template<VersionType version>
	struct AccountPublicKeyNotification;

	template<>
	struct AccountPublicKeyNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Core_Register_Account_Public_Key_v1_Notification;

	public:
		/// Creates a notification around \a publicKey.
		explicit AccountPublicKeyNotification(const Key& publicKey)
				: Notification(Notification_Type, sizeof(AccountPublicKeyNotification<1>))
				, PublicKey(publicKey)
		{}

	public:
		/// Public key.
		const Key& PublicKey;
	};

	// endregion

	// region balance

	/// A basic balance notification.
	template<typename TDerivedNotification>
	struct BasicBalanceNotification : public Notification {
	public:
		/// Creates a notification around \a sender, \a mosaicId and \a amount.
		BasicBalanceNotification(const Key& sender, UnresolvedMosaicId mosaicId, UnresolvedAmount amount)
				: Notification(TDerivedNotification::Notification_Type, sizeof(TDerivedNotification))
				, Sender(sender)
				, MosaicId(mosaicId)
				, Amount(amount)
		{}

		/// Creates a notification around \a sender, \a recipient, \a mosaicId and \a amount.
		BasicBalanceNotification(const Key& sender, UnresolvedMosaicId mosaicId, catapult::Amount amount)
				: BasicBalanceNotification(sender, mosaicId, UnresolvedAmount(amount.unwrap()))
		{}

	public:
		/// Sender.
		const Key& Sender;

		/// Mosaic id.
		UnresolvedMosaicId MosaicId;

		/// Amount.
		UnresolvedAmount Amount;
	};

	/// Notifies a balance transfer from sender to recipient.
	template<VersionType version>
	struct BalanceTransferNotification;

	template<>
	struct BalanceTransferNotification<1> : public BasicBalanceNotification<BalanceTransferNotification<1>> {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Core_Balance_Transfer_v1_Notification;

	public:
		/// Creates a notification around \a sender, \a recipient, \a mosaicId and \a unresolved amount.
		BalanceTransferNotification(
				const Key& sender,
				const UnresolvedAddress& recipient,
				UnresolvedMosaicId mosaicId,
				catapult::UnresolvedAmount unresolvedAmount)
				: BasicBalanceNotification(sender, mosaicId, unresolvedAmount)
				, Recipient(recipient)
		{}

		/// Creates a notification around \a sender, \a recipient, \a mosaicId and \a amount.
		BalanceTransferNotification(
				const Key& sender,
				const UnresolvedAddress& recipient,
				UnresolvedMosaicId mosaicId,
				catapult::Amount amount)
				: BasicBalanceNotification(sender, mosaicId, amount)
				, Recipient(recipient)
		{}

	public:
		/// Recipient.
		UnresolvedAddress Recipient;
	};

	/// Notifies a balance debit by sender.
	template<VersionType version>
	struct BalanceDebitNotification;

	template<>
	struct BalanceDebitNotification<1> : public BasicBalanceNotification<BalanceDebitNotification<1>> {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Core_Balance_Debit_v1_Notification;

	public:
		using BasicBalanceNotification<BalanceDebitNotification<1>>::BasicBalanceNotification;
	};

	/// Notifies a balance credit to sender.
	template<VersionType version>
	struct BalanceCreditNotification;

	template<>
	struct BalanceCreditNotification<1> : public BasicBalanceNotification<BalanceCreditNotification<1>> {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Core_Balance_Credit_v1_Notification;

	public:
		using BasicBalanceNotification<BalanceCreditNotification<1>>::BasicBalanceNotification;
	};

	// endregion

	// region entity

	/// Notifies the arrival of an entity.
	template<VersionType version>
	struct EntityNotification;

	template<>
	struct EntityNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Core_Entity_v1_Notification;

	public:
		/// Creates an entity notification around \a networkIdentifier, \a minVersion, \a maxVersion and \a entityVersion.
		EntityNotification(
				model::NetworkIdentifier networkIdentifier,
				model::EntityType entityType,
                VersionType entityVersion)
				: Notification(Notification_Type, sizeof(EntityNotification<1>))
				, NetworkIdentifier(networkIdentifier)
				, EntityType(entityType)
				, EntityVersion(entityVersion)
		{}

	public:
		/// Network identifier.
		model::NetworkIdentifier NetworkIdentifier;

		/// Entity type.
		model::EntityType EntityType;

		/// Entity version.
        VersionType EntityVersion;
	};

	// endregion

	// region block

	/// Notifies the arrival of a block.
	template<VersionType version>
	struct BlockNotification;

	template<>
	struct BlockNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Core_Block_v1_Notification;

	public:
		/// Creates a block notification around \a signer, \a beneficiary, \a timestamp and \a difficulty.
		 BlockNotification(
			const Key& signer,
			const Key& beneficiary,
			Timestamp timestamp,
			Difficulty difficulty,
			uint32_t feeInterest,
			uint32_t feeInterestDenominator)
				: Notification(Notification_Type, sizeof(BlockNotification<1>))
				, Signer(signer)
				, Beneficiary(beneficiary)
				, Timestamp(timestamp)
				, Difficulty(difficulty)
				, NumTransactions(0)
				, FeeInterest(feeInterest)
				, FeeInterestDenominator(feeInterestDenominator)
		{}

	public:
		/// Block signer.
		const Key& Signer;

		/// Beneficiary.
		const Key& Beneficiary;

		/// Block timestamp.
		catapult::Timestamp Timestamp;

		/// Block difficulty.
		catapult::Difficulty Difficulty;

		/// Total block fee.
		Amount TotalFee;

		/// Number of block transactions.
		uint32_t NumTransactions;

		/// The part of the transaction fee harvester is willing to get.
		uint32_t FeeInterest;

		/// Denominator of the transaction fee.
		uint32_t FeeInterestDenominator;
	};

	/// Notifies the cosignatures of a block.
	template<VersionType version>
	struct BlockCosignaturesNotification;

	template<>
	struct BlockCosignaturesNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Core_Block_Cosignatures_v1_Notification;

	public:
		/// Creates a block notification around \a signer, \a numCosignatures, \a pCosignatures,
		/// \a feeInterest and \a feeInterestDenominator.
		 BlockCosignaturesNotification(
			const Key& signer,
			int16_t round,
			size_t numCosignatures,
			const Cosignature* pCosignatures,
			uint32_t feeInterest,
			uint32_t feeInterestDenominator)
				: Notification(Notification_Type, sizeof(BlockCosignaturesNotification<1>))
				, Signer(signer)
				, Round(round)
				, NumCosignatures(numCosignatures)
				, CosignaturesPtr(pCosignatures)
				, FeeInterest(feeInterest)
				, FeeInterestDenominator(feeInterestDenominator)
		{}

	public:
		/// Block signer.
		const Key& Signer;

		/// Committee round (number of attempts to generate this block).
		int16_t Round;

		/// Number of block cosignatures.
		size_t NumCosignatures;

		/// Block cosignatures.
		const Cosignature* CosignaturesPtr;

		/// The part of the transaction fee harvester is willing to get.
		uint32_t FeeInterest;

		/// Denominator of the transaction fee.
		uint32_t FeeInterestDenominator;
	};

	// endregion

	// region transaction

	/// Notifies the arrival of a transaction.
	template<VersionType version>
	struct TransactionNotification;

	template<>
	struct TransactionNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Core_Transaction_v1_Notification;

	public:
		/// Creates a transaction notification around \a signer, \a transactionHash, \a transactionType and \a deadline.
		TransactionNotification(const Key& signer, const Hash256& transactionHash, EntityType transactionType, Timestamp deadline)
				: Notification(Notification_Type, sizeof(TransactionNotification<1>))
				, Signer(signer)
				, TransactionHash(transactionHash)
				, TransactionType(transactionType)
				, Deadline(deadline)
		{}

	public:
		/// Transaction signer.
		const Key& Signer;

		/// Transaction hash.
		const Hash256& TransactionHash;

		/// Transaction type.
		EntityType TransactionType;

		/// Transaction deadline.
		Timestamp Deadline;
	};

	/// Notifies the arrival of a transaction deadline.
	template<VersionType version>
	struct TransactionDeadlineNotification;

	template<>
	struct TransactionDeadlineNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Core_Transaction_Deadline_v1_Notification;

	public:
		/// Creates a transaction deadline notification around \a deadline and \a maxLifetime.
		TransactionDeadlineNotification(Timestamp deadline, utils::TimeSpan maxLifetime)
				: Notification(Notification_Type, sizeof(TransactionDeadlineNotification<1>))
				, Deadline(deadline)
				, MaxLifetime(maxLifetime)
		{}

	public:
		/// Transaction deadline.
		Timestamp Deadline;

		/// Custom maximum transaction lifetime.
		/// \note If \c 0, default network-specific maximum will be used.
		utils::TimeSpan MaxLifetime;
	};

	/// Notifies the arrival of a transaction fee.
	template<VersionType version>
	struct TransactionFeeNotification;

	template<>
	struct TransactionFeeNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Core_Transaction_Fee_v1_Notification;

	public:
		/// Creates a transaction fee notification around \a transactionSize, \a fee and \a maxFee.
		TransactionFeeNotification(uint32_t transactionSize, Amount fee, Amount maxFee)
				: Notification(Notification_Type, sizeof(TransactionFeeNotification<1>))
				, TransactionSize(transactionSize)
				, Fee(fee)
				, MaxFee(maxFee)
		{}

	public:
		/// Transaction size.
		uint32_t TransactionSize;

		/// Transaction fee.
		Amount Fee;

		// Maximum transaction fee.
		Amount MaxFee;
	};

	// endregion

	// region signature

	/// Notifies the presence of a signature.
	template<VersionType version>
	struct SignatureNotification;

	template<>
	struct SignatureNotification<1> : public Notification {
	public:
		/// Replay protection modes.
		enum class ReplayProtectionMode { Enabled, Disabled };
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Core_Signature_v1_Notification;

	public:
		/// Creates a signature notification around \a signer, \a signature and \a data with optional replay protection mode
		/// (\a dataReplayProtectionMode) applied to data.
		SignatureNotification(
				const Key& signer,
				const Signature& signature,
				const RawBuffer& data,
				ReplayProtectionMode dataReplayProtectionMode = ReplayProtectionMode::Disabled)
				: Notification(Notification_Type, sizeof(SignatureNotification<1>))
				, Signer(signer)
				, Signature(signature)
				, Data(data)
				, DataReplayProtectionMode(dataReplayProtectionMode)
		{}

	public:
		/// Signer.
		const Key& Signer;

		/// Signature.
		const catapult::Signature& Signature;

		/// Signed data.
		RawBuffer Data;

		/// Replay protection mode applied to data.
		ReplayProtectionMode DataReplayProtectionMode;
	};

	// endregion

	// region address interaction

	/// Notifies that a source address interacts with participant addresses.
	/// \note This notification cannot be used by an observer.
	template<VersionType version>
	struct AddressInteractionNotification;

	template<>
	struct AddressInteractionNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Core_Address_Interaction_v1_Notification;

	public:
		/// Creates a notification around \a source, \a transactionType and \a participantsByAddress.
		AddressInteractionNotification(const Key& source, EntityType transactionType, const UnresolvedAddressSet& participantsByAddress)
				: AddressInteractionNotification(source, transactionType, participantsByAddress, {})
		{}

		/// Creates a notification around \a source, \a transactionType, \a participantsByAddress and \a participantsByKey.
		AddressInteractionNotification(
				const Key& source,
				EntityType transactionType,
				const UnresolvedAddressSet& participantsByAddress,
				const utils::KeySet& participantsByKey)
				: Notification(Notification_Type, sizeof(AddressInteractionNotification<1>))
				, Source(source)
				, TransactionType(transactionType)
				, ParticipantsByAddress(participantsByAddress)
				, ParticipantsByKey(participantsByKey)
		{}

	public:
		/// Source.
		Key Source;

		/// Transaction type.
		EntityType TransactionType;

		/// Participants given by address.
		UnresolvedAddressSet ParticipantsByAddress;

		/// Participants given by public key.
		utils::KeySet ParticipantsByKey;
	};

	// endregion

	// region mosaic required

	/// Notification of a required mosaic.
	template<VersionType version>
	struct MosaicRequiredNotification;

	template<>
	struct MosaicRequiredNotification<1> : public Notification {
	public:
		/// Mosaic types.
		enum class MosaicType { Resolved, Unresolved };

	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Core_Mosaic_Required_v1_Notification;

	public:
		/// Creates a notification around \a signer and \a mosaicId.
		MosaicRequiredNotification(const Key& signer, MosaicId mosaicId)
				: Notification(Notification_Type, sizeof(MosaicRequiredNotification<1>))
				, Signer(signer)
				, MosaicId(mosaicId)
				, ProvidedMosaicType(MosaicType::Resolved)
		{}

		/// Creates a notification around \a signer and \a mosaicId.
		MosaicRequiredNotification(const Key& signer, UnresolvedMosaicId mosaicId)
				: Notification(Notification_Type, sizeof(MosaicRequiredNotification<1>))
				, Signer(signer)
				, UnresolvedMosaicId(mosaicId)
				, ProvidedMosaicType(MosaicType::Unresolved)
		{}

	public:
		/// Signer.
		const Key& Signer;

		/// Mosaic id (resolved).
		catapult::MosaicId MosaicId;

		/// Mosaic id (unresolved).
		catapult::UnresolvedMosaicId UnresolvedMosaicId;

		/// Type of mosaic provided and attached to this notification.
		MosaicType ProvidedMosaicType;
	};

	// endregion

	// region source change

	/// Notification of a source change.
	template<VersionType version>
	struct SourceChangeNotification;

	template<>
	struct SourceChangeNotification<1> : public Notification {
	public:
		/// Source change types.
		enum class SourceChangeType { Absolute, Relative };

	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Core_Source_Change_v1_Notification;

	public:
		/// Creates a notification around \a primaryChangeType, \a primaryId, \a secondaryChangeType and \a secondaryId.
		SourceChangeNotification(
				SourceChangeType primaryChangeType,
				uint32_t primaryId,
				SourceChangeType secondaryChangeType,
				uint32_t secondaryId)
				: Notification(Notification_Type, sizeof(SourceChangeNotification<1>))
				, PrimaryChangeType(primaryChangeType)
				, PrimaryId(primaryId)
				, SecondaryChangeType(secondaryChangeType)
				, SecondaryId(secondaryId)
		{}

	public:
		/// Type of primary source change.
		SourceChangeType PrimaryChangeType;

		/// Primary source (e.g. index within block).
		uint32_t PrimaryId;

		/// Type of secondary source change.
		SourceChangeType SecondaryChangeType;

		/// Secondary source (e.g. index within aggregate).
		uint32_t SecondaryId;
	};

	// endregion

	// region configuration change

	/// Notification of a plugin configuration change.
	template<VersionType version>
	struct PluginConfigNotification;

	template<>
	struct PluginConfigNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Core_Plugin_Config_v1_Notification;

	public:
		/// Creates a notification around \a primaryId, \a secondaryId and \a changeType.
		explicit PluginConfigNotification(const std::string& name, const utils::ConfigurationBag& bag)
			: Notification(Notification_Type, sizeof(PluginConfigNotification<1>))
			, Name(name)
			, Bag(bag)
		{}

	public:
		/// Plugin name.
		const std::string& Name;

		/// Plugin configuration bag.
		const utils::ConfigurationBag& Bag;
	};

	// endregion
}}
