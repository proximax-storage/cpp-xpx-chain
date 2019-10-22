/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Notifications.h"
#include "src/model/ServiceTypes.h"
#include "catapult/utils/MemoryUtils.h"

namespace catapult { namespace model {

	/// Defines a prepare drive notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, Prepare_Drive_v1, 0x0001);

	/// Defines a drive file system notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, Drive_File_System_v1, 0x0002);

	/// Defines a drive deposit notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, Join_To_Drive_v1, 0x0003);

	/// Defines a drive files deposit notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, Files_Deposit_v1, 0x0004);

	/// Defines a drive verification notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, Drive_Verification_v1, 0x0005);

	/// Notification of a drive prepare.
	template<VersionType version>
	struct PrepareDriveNotification;

	template<>
	struct PrepareDriveNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_Prepare_Drive_v1_Notification;

	public:
		explicit PrepareDriveNotification(
			const Key& drive,
			const Key& owner,
            const BlockDuration& duration,
			const BlockDuration& billingPeriod,
			const Amount& billingPrice,
			uint64_t size,
			uint16_t replicas,
			uint8_t minReplicators,
			uint8_t percentApprovers)
			: Notification(Notification_Type, sizeof(PrepareDriveNotification<1>))
			, DriveKey(drive)
			, Owner(owner)
			, Duration(duration)
			, BillingPeriod(billingPeriod)
			, BillingPrice(billingPrice)
			, DriveSize(size)
			, Replicas(replicas)
			, MinReplicators(minReplicators)
			, PercentApprovers(percentApprovers)
		{}

	public:
		/// Public key of the drive multisig account.
		Key DriveKey;

		/// Public key of the drive owner.
		Key Owner;

        /// Duration of drive.
        BlockDuration Duration;

		/// Billing period of drive.
		BlockDuration BillingPeriod;

		/// Billing price of drive in storage units.
		Amount BillingPrice;

		/// The size of the drive.
		uint64_t DriveSize;

		/// The number of drive replicas.
		uint16_t Replicas;

		/// Minimal count of replicator to send transaction from name of drive.
		int8_t MinReplicators;

		/// Percent of approves from replicators to apply transaction.
		int8_t PercentApprovers;
	};

	/// Notification of a drive deposit.
	template<VersionType version>
	struct JoinToDriveNotification;

	template<>
	struct JoinToDriveNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_Join_To_Drive_v1_Notification;

	public:
		explicit JoinToDriveNotification(
			const Key& drive,
			const Key& replicator)
			: Notification(Notification_Type, sizeof(JoinToDriveNotification<1>))
			, DriveKey(drive)
			, Replicator(replicator)
		{}

	public:
		/// Public key of the drive multisig account.
		const Key& DriveKey;

		/// Replicator public key.
        const Key& Replicator;
	};

	struct DriveDeposit {
		const Key& DriveKey;
	};

	struct FileDeposit {
		const Key& DriveKey;
		const Hash256& FileHash;
	};

	struct FileUpload {
		const Key& DriveKey;
		const uint64_t& FileSize;
	};

	/// Notification of a drive deposit.
	template<VersionType version>
	struct DriveFileSystemNotification;

	template<>
	struct DriveFileSystemNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_Drive_File_System_v1_Notification;

	public:
		explicit DriveFileSystemNotification(
			const Key& drive,
			const Key& signer,
			const Hash256& rootHash,
			const Hash256& xorRootHash,
			const uint16_t& addActionsCount,
			const model::AddAction* addActionsPtr,
			const uint16_t& removeActionsCount,
			const model::RemoveAction* removeActionsPtr)
			: Notification(Notification_Type, sizeof(DriveFileSystemNotification<1>))
			, DriveKey(drive)
			, Signer(signer)
			, RootHash(rootHash)
			, XorRootHash(xorRootHash)
			, AddActionsCount(addActionsCount)
			, AddActionsPtr(addActionsPtr)
			, RemoveActionsCount(removeActionsCount)
			, RemoveActionsPtr(removeActionsPtr)
		{}

	public:
		/// Key of drive.
		const Key& DriveKey;

		/// Signer of transaction.
		const Key& Signer;

		/// A new RootHash of drive.
		const Hash256& RootHash;

		/// Xor of a new RootHash of drive with previous RootHash.
		const Hash256& XorRootHash;

		/// Count of add actions.
		const uint16_t& AddActionsCount;

		/// Actions to add files to drive.
		const model::AddAction* AddActionsPtr;

		/// Count of remove actions.
		const uint16_t& RemoveActionsCount;

		/// Actions to remove files from drive.
		const model::RemoveAction* RemoveActionsPtr;
	};

	/// Notification of a drive files deposit.
	template<VersionType version>
	struct FilesDepositNotification;

	template<>
	struct FilesDepositNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_Files_Deposit_v1_Notification;

	public:
		explicit FilesDepositNotification(
			const Key& drive,
			const Key& replicator,
			uint16_t filesCount,
			const File* filesPtr)
			: Notification(Notification_Type, sizeof(FilesDepositNotification<1>))
			, DriveKey(drive)
			, Replicator(replicator)
			, FilesCount(filesCount)
			, FilesPtr(filesPtr)
		{}

	public:
		/// Public key of the drive multisig account.
		Key DriveKey;

		/// Replicator public key.
		Key Replicator;

		/// Files count.
		uint16_t FilesCount;

		/// Files pointer.
		const File* FilesPtr;
	};

	/// Notification of a drive verification.
	template<VersionType version>
	struct DriveVerificationNotification;

	template<>
	struct DriveVerificationNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_Drive_Verification_v1_Notification;

	public:
		explicit DriveVerificationNotification(
			const Key& drive,
			const Key& replicator)
			: Notification(Notification_Type, sizeof(DriveVerificationNotification<1>))
			, Drive(drive)
			, Replicator(replicator)
		{}

	public:
		/// Public key of the drive multisig account.
		Key Drive;

		/// Replicator public key.
		Key Replicator;
	};
}}
