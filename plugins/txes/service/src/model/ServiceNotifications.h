/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Notifications.h"
#include "src/model/ServiceTypes.h"
#include "catapult/utils/MemoryUtils.h"
#include <vector>

namespace catapult { namespace model {

	/// Defines a prepare drive notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, Prepare_Drive_v1, 0x0001);

	/// Defines a drive file system notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, Drive_File_System_v1, 0x0002);

	/// Defines a drive deposit notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, Join_To_Drive_v1, 0x0003);

	/// Defines a drive files deposit notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, Files_Deposit_v1, 0x0004);

	/// Defines a start drive verification notification type.
	DEFINE_NOTIFICATION_TYPE(Validator, Service, Start_Drive_Verification_v1, 0x0005);

	/// Defines an end drive verification notification type.
	DEFINE_NOTIFICATION_TYPE(Validator, Service, End_Drive_Verification_v1, 0x0006);

	/// Defines a drive verification payment notification type.
	DEFINE_NOTIFICATION_TYPE(Observer, Service, Drive_Verification_Payment_v1, 0x0007);

	/// Defines a drive notification type.
	DEFINE_NOTIFICATION_TYPE(Validator, Service, Drive_v1, 0x0008);

	/// Defines a end drive notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, End_Drive_v1, 0x0009);

	/// Defines a drive files reward notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, DriveFilesReward_v1, 0x000A);

	/// Defines a failed block hashes notification type.
	DEFINE_NOTIFICATION_TYPE(Validator, Service, Failed_Block_Hashes_v1, 0x000B);

	/// Defines a start file download notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, StartFileDownload_v1, 0x000C);

	/// Defines an end file download notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, EndFileDownload_v1, 0x000D);

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
			uint16_t minReplicators,
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
		uint16_t MinReplicators;

		/// Percent of approves from replicators to apply transaction.
		uint8_t PercentApprovers;
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
		Key DriveKey;

		/// Replicator public key.
		Key Replicator;
	};

	struct DriveDeposit : public UnresolvedAmountData {
	public:
		DriveDeposit(const Key& driveKey)
			: DriveKey(driveKey)
		{}

	public:
		const Key& DriveKey;
	};

	struct FileDeposit : public UnresolvedAmountData {
	public:
		FileDeposit(const Key& driveKey, const Hash256& fileHash)
			: DriveKey(driveKey)
			, FileHash(fileHash)
		{}

	public:
		Key DriveKey;
		Hash256 FileHash;
	};

	struct FileUpload : public UnresolvedAmountData {
	public:
		FileUpload(const Key& driveKey, const uint64_t& fileSize)
			: DriveKey(driveKey)
			, FileSize(fileSize)
		{}

	public:
		Key DriveKey;
		uint64_t FileSize;
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
		Key DriveKey;

		/// Signer of transaction.
		Key Signer;

		/// A new RootHash of drive.
		Hash256 RootHash;

		/// Xor of a new RootHash of drive with previous RootHash.
		Hash256 XorRootHash;

		/// Count of add actions.
		uint16_t AddActionsCount;

		/// Actions to add files to drive.
		const model::AddAction* AddActionsPtr;

		/// Count of remove actions.
		uint16_t RemoveActionsCount;

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
	struct StartDriveVerificationNotification;

	template<>
	struct StartDriveVerificationNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_Start_Drive_Verification_v1_Notification;

	public:
		explicit StartDriveVerificationNotification(
			const Key& drive,
			const Key& initiator)
			: Notification(Notification_Type, sizeof(StartDriveVerificationNotification<1>))
			, DriveKey(drive)
			, Initiator(initiator)
		{}

	public:
		/// Public key of the drive multisig account.
		Key DriveKey;

		/// Verification initiator public key.
		Key Initiator;
	};

	/// Base notification of an end drive verification.
	struct BaseEndDriveVerificationNotification : public Notification {
	public:
		explicit BaseEndDriveVerificationNotification(
			NotificationType type,
			const Key& drive,
			uint16_t failureCount,
			const Key* pFailedReplicators)
			: Notification(type, sizeof(BaseEndDriveVerificationNotification))
			, DriveKey(drive)
			, FailureCount(failureCount)
			, FailedReplicatorsPtr(pFailedReplicators)
		{}

	public:
		/// Public key of the drive multisig account.
		Key DriveKey;

		/// Count of verification failures.
		uint16_t FailureCount;

		/// Verification failures.
		const Key* FailedReplicatorsPtr;
	};

	/// Notification of an end drive verification.
	template<VersionType version>
	struct EndDriveVerificationNotification;

	template<>
	struct EndDriveVerificationNotification<1> : public BaseEndDriveVerificationNotification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_End_Drive_Verification_v1_Notification;

	public:
		explicit EndDriveVerificationNotification(
			const Key& drive,
			uint16_t failureCount,
			const Key* pFailedReplicators)
			: BaseEndDriveVerificationNotification(Notification_Type, drive, failureCount, pFailedReplicators)
		{}
	};

	/// Notification of a drive verification payment.
	template<VersionType version>
	struct DriveVerificationPaymentNotification;

	template<>
	struct DriveVerificationPaymentNotification<1> : public BaseEndDriveVerificationNotification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_Drive_Verification_Payment_v1_Notification;

	public:
		explicit DriveVerificationPaymentNotification(
			const Key& drive,
			uint16_t failureCount,
			const Key* pFailedReplicators)
			: BaseEndDriveVerificationNotification(Notification_Type, drive, failureCount, pFailedReplicators)
		{}
	};

	/// Notification of a drive.
	template<VersionType version>
	struct DriveNotification;

	template<>
	struct DriveNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_Drive_v1_Notification;

	public:
		explicit DriveNotification(const Key& drive, const model::EntityType& type)
			: Notification(Notification_Type, sizeof(DriveNotification<1>))
			, DriveKey(drive)
			, TransactionType(type)
		{}

	public:
		/// Public key of the drive multisig account.
		Key DriveKey;

		/// Transactions type.
        model::EntityType TransactionType;
	};

	/// Notification of an end drive.
	template<VersionType version>
	struct EndDriveNotification;

	template<>
	struct EndDriveNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_End_Drive_v1_Notification;

	public:
		explicit EndDriveNotification(const Key& drive, const Key& signer)
				: Notification(Notification_Type, sizeof(EndDriveNotification<1>))
				, DriveKey(drive)
				, Signer(signer)
		{}

	public:
		/// Public key of the drive multisig account.
		Key DriveKey;

		/// Public key of the signer.
		Key Signer;
	};

	/// Notification of a drive files reward.
	template<VersionType version>
	struct DriveFilesRewardNotification;

	template<>
	struct DriveFilesRewardNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_DriveFilesReward_v1_Notification;

	public:
		explicit DriveFilesRewardNotification(const Key& key, const UploadInfo* ptr, uint32_t count)
				: Notification(Notification_Type, sizeof(DriveFilesRewardNotification<1>))
				, DriveKey(key)
				, UploadInfoPtr(ptr)
				, UploadInfosCount(count)
		{}

	public:
		/// Public key of the drive multisig account.
		Key DriveKey;

		/// Vector of deleted files.
		const UploadInfo* UploadInfoPtr;

		/// Upload infos count
		uint32_t UploadInfosCount;
	};

	/// Notification of block hashes that failed verification.
	template<VersionType version>
	struct FailedBlockHashesNotification;

	template<>
	struct FailedBlockHashesNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_Failed_Block_Hashes_v1_Notification;

	public:
		explicit FailedBlockHashesNotification(const uint16_t& blockHashCount, const Hash256* pBlockHashes)
				: Notification(Notification_Type, sizeof(FailedBlockHashesNotification<1>))
				, BlockHashCount(blockHashCount)
				, BlockHashesPtr(pBlockHashes)
		{}

	public:
		/// Count of the hashes of the blocks that failed verification.
		uint16_t BlockHashCount;

		/// Array of the hashes of the blocks that failed verification.
		const Hash256* BlockHashesPtr;
	};

	/// Base notification of a file download.
	struct BaseFileDownloadNotification : public Notification {
	public:
		explicit BaseFileDownloadNotification(
			NotificationType type,
			const Key& fileRecipient,
			const Hash256& operationToken,
			const DownloadAction* ptr,
			uint16_t count)
				: Notification(type, sizeof(BaseFileDownloadNotification))
				, FileRecipient(fileRecipient)
				, OperationToken(operationToken)
				, FilesPtr(ptr)
				, FileCount(count)
		{}

	public:
		/// File recipient.
		Key FileRecipient;

		/// Operation token.
		Hash256 OperationToken;

		/// Array of files to download.
		const DownloadAction* FilesPtr;

		/// Download file count.
		uint16_t FileCount;
	};

	/// Notification of start file download.
	template<VersionType version>
	struct StartFileDownloadNotification;

	template<>
	struct StartFileDownloadNotification<1> : public BaseFileDownloadNotification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_StartFileDownload_v1_Notification;

	public:
		explicit StartFileDownloadNotification(const Key& driveKey, const Key& fileRecipient, const Hash256& operationToken, const DownloadAction* ptr, uint16_t count)
				: BaseFileDownloadNotification(Notification_Type, fileRecipient, operationToken, ptr, count)
				, DriveKey(driveKey)
		{}

	public:
		/// Public key of the drive multisig account.
		Key DriveKey;
	};

	/// Notification of end file download.
	template<VersionType version>
	struct EndFileDownloadNotification;

	template<>
	struct EndFileDownloadNotification<1> : public BaseFileDownloadNotification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_EndFileDownload_v1_Notification;

	public:
		explicit EndFileDownloadNotification(const Key& fileRecipient, const Hash256& operationToken, const DownloadAction* ptr, uint16_t count)
				: BaseFileDownloadNotification(Notification_Type, fileRecipient, operationToken, ptr, count)
		{}
	};
}}
