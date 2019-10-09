/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Notifications.h"
#include "catapult/utils/MemoryUtils.h"

namespace catapult { namespace model {

	/// Defines a transfer mosaics notification type.
	DEFINE_NOTIFICATION_TYPE(Validator, Service, Transfer_Mosaics_v1, 0x0001);

	/// Defines a drive notification type.
	DEFINE_NOTIFICATION_TYPE(Validator, Service, Drive_v1, 0x0002);

	/// Defines a replicator notification type.
	DEFINE_NOTIFICATION_TYPE(Validator, Service, Replicator_v1, 0x0003);

	/// Defines a replicator notification type.
	DEFINE_NOTIFICATION_TYPE(Validator, Service, MosaicId_v1, 0x0004);

	/// Defines a prepare drive notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, Prepare_Drive_v1, 0x0005);

	/// Defines a drive prolongation notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, Drive_Prolongation_v1, 0x0006);

	/// Defines a drive deposit notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, Drive_Deposit_v1, 0x0007);

	/// Defines a drive deposit notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, Drive_Deposit_Return_v1, 0x0008);

	/// Defines a drive payment notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, Drive_Payment_v1, 0x0009);

	/// Defines a drive file deposit notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, File_Deposit_v1, 0x000A);

	/// Defines a drive file deposit notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, File_Deposit_Return_v1, 0x000B);

	/// Defines a drive file payment notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, File_Payment_v1, 0x000C);

	/// Defines a drive verification notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, Drive_Verification_v1, 0x000D);

	/// Defines create drive directory notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, Create_Directory_v1, 0x000E);

	/// Defines remove drive directory notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, Remove_Directory_v1, 0x000F);

	/// Defines upload drive file notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, Upload_File_v1, 0x0010);

	/// Defines delete drive file notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, Delete_File_v1, 0x0011);

	/// Defines move drive file notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, Move_File_v1, 0x0012);

	/// Defines copy drive file notification type.
	DEFINE_NOTIFICATION_TYPE(All, Service, Copy_File_v1, 0x0013);

	/// Notification of a mosaic transfer.
	template<VersionType version>
	struct TransferMosaicsNotification;

	template<>
	struct TransferMosaicsNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_Transfer_Mosaics_v1_Notification;

	public:
		/// Creates a notification around \a mosaicsCount and \a pMosaics.
		explicit TransferMosaicsNotification(DriveActionType actionType, uint8_t mosaicsCount, const UnresolvedMosaic* pMosaics)
				: Notification(Notification_Type, sizeof(TransferMosaicsNotification<1>))
				, ActionType(actionType)
				, MosaicsCount(mosaicsCount)
				, MosaicsPtr(pMosaics)
		{}

	public:
		/// Action type.
		DriveActionType ActionType;

		/// Number of mosaics.
		uint8_t MosaicsCount;

		/// Const pointer to the first mosaic.
		const UnresolvedMosaic* MosaicsPtr;
	};

	/// Notification of a drive key.
	template<VersionType version>
	struct DriveNotification;

	template<>
	struct DriveNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_Drive_v1_Notification;

	public:
		explicit DriveNotification(const Key& drive)
			: Notification(Notification_Type, sizeof(DriveNotification<1>))
			, Drive(drive)
		{}

	public:
		/// Public key of the drive multisig account.
		Key Drive;
	};

	/// Notification of a replicator key.
	template<VersionType version>
	struct ReplicatorNotification;

	template<>
	struct ReplicatorNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_Replicator_v1_Notification;

	public:
		explicit ReplicatorNotification(const Key& drive, const Key& replicator)
			: Notification(Notification_Type, sizeof(ReplicatorNotification<1>))
			, Drive(drive)
			, Replicator(replicator)
		{}

	public:
		/// Public key of the drive multisig account.
		Key Drive;

		/// Public key of the replicator.
		Key Replicator;
	};

	/// Notification of a replicator key.
	template<VersionType version>
	struct MosaicIdNotification;

	template<>
	struct MosaicIdNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_MosaicId_v1_Notification;

	public:
		explicit MosaicIdNotification(const UnresolvedMosaicId& paidMosaicId, const MosaicId& validMosaicId)
			: Notification(Notification_Type, sizeof(MosaicIdNotification<1>))
			, PaidMosaicId(paidMosaicId)
			, ValidMosaicId(validMosaicId)
		{}

	public:
		/// Paid mosaic id.
		UnresolvedMosaicId PaidMosaicId;

		/// Valid mosaic id.
		MosaicId ValidMosaicId;
	};

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
			const BlockDuration& duration,
			uint64_t size,
			uint16_t replicas,
			const Key& customer,
			const model::UnresolvedMosaic& deposit)
			: Notification(Notification_Type, sizeof(PrepareDriveNotification<1>))
			, Drive(drive)
			, Duration(duration)
			, Size(size)
			, Replicas(replicas)
			, Customer(customer)
			, Deposit(deposit)
		{}

	public:
		/// Public key of the drive multisig account.
		Key Drive;

		/// Relative change of the duration of the drive in blocks.
		BlockDuration Duration;

		/// The size of the drive.
		uint64_t Size;

		/// The number of drive replicas.
		uint16_t Replicas;

		/// Customer public key.
		Key Customer;

		/// Customer deposit.
		model::UnresolvedMosaic Deposit;
	};

	/// Notification of a drive prolongation.
	template<VersionType version>
	struct DriveProlongationNotification;

	template<>
	struct DriveProlongationNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_Drive_Prolongation_v1_Notification;

	public:
		explicit DriveProlongationNotification(
			const Key& drive,
			const BlockDuration& duration,
			const Key& customer,
			const model::UnresolvedMosaic& deposit)
			: Notification(Notification_Type, sizeof(DriveProlongationNotification<1>))
			, Drive(drive)
			, Duration(duration)
			, Customer(customer)
			, Deposit(deposit)
		{}

	public:
		/// Public key of the drive multisig account.
		Key Drive;

		/// Relative change of the duration of the drive in blocks.
		BlockDuration Duration;

		/// Customer public key.
		Key Customer;

		/// Customer deposit.
		model::UnresolvedMosaic Deposit;
	};

	/// Notification of a drive deposit.
	template<VersionType version>
	struct DriveDepositNotification;

	template<>
	struct DriveDepositNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_Drive_Deposit_v1_Notification;

	public:
		explicit DriveDepositNotification(
			const Key& drive,
			const Key& replicator,
			const model::UnresolvedMosaic& deposit)
			: Notification(Notification_Type, sizeof(DriveDepositNotification<1>))
			, Drive(drive)
			, Replicator(replicator)
			, Deposit(deposit)
		{}

	public:
		/// Public key of the drive multisig account.
		Key Drive;

		/// Replicator public key.
		Key Replicator;

		/// Replicator deposit.
		model::UnresolvedMosaic Deposit;
	};

	/// Notification of a drive deposit.
	template<VersionType version>
	struct DriveDepositReturnNotification;

	template<>
	struct DriveDepositReturnNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_Drive_Deposit_Return_v1_Notification;

	public:
		explicit DriveDepositReturnNotification(
			const Key& drive,
			const Key& replicator,
			const model::UnresolvedMosaic& deposit)
			: Notification(Notification_Type, sizeof(DriveDepositReturnNotification<1>))
			, Drive(drive)
			, Replicator(replicator)
			, Deposit(deposit)
		{}

	public:
		/// Public key of the drive multisig account.
		Key Drive;

		/// Replicator public key.
		Key Replicator;

		/// Replicator deposit.
		model::UnresolvedMosaic Deposit;
	};

	/// Notification of a drive file deposit.
	template<VersionType version>
	struct FileDepositNotification;

	template<>
	struct FileDepositNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_File_Deposit_v1_Notification;

	public:
		explicit FileDepositNotification(
			const Key& drive,
			const Key& replicator,
			const model::UnresolvedMosaic& deposit,
			Hash256 fileHash)
			: Notification(Notification_Type, sizeof(FileDepositNotification<1>))
			, Drive(drive)
			, Replicator(replicator)
			, Deposit(deposit)
			, FileHash(fileHash)
		{}

	public:
		/// Public key of the drive multisig account.
		Key Drive;

		/// Replicator public key.
		Key Replicator;

		/// Replicator deposit.
		model::UnresolvedMosaic Deposit;

		/// File hash.
		Hash256 FileHash;
	};

	/// Notification of a drive file deposit.
	template<VersionType version>
	struct FileDepositReturnNotification;

	template<>
	struct FileDepositReturnNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_File_Deposit_Return_v1_Notification;

	public:
		explicit FileDepositReturnNotification(
			const Key& drive,
			const Key& replicator,
			const model::UnresolvedMosaic& deposit,
			Hash256 fileHash)
			: Notification(Notification_Type, sizeof(FileDepositReturnNotification<1>))
			, Drive(drive)
			, Replicator(replicator)
			, Deposit(deposit)
			, FileHash(fileHash)
		{}

	public:
		/// Public key of the drive multisig account.
		Key Drive;

		/// Replicator public key.
		Key Replicator;

		/// Replicator deposit.
		model::UnresolvedMosaic Deposit;

		/// File hash.
		Hash256 FileHash;
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
			const Key& replicator,
			const model::UnresolvedMosaic& deposit)
			: Notification(Notification_Type, sizeof(DriveVerificationNotification<1>))
			, Drive(drive)
			, Replicator(replicator)
			, Deposit(deposit)
		{}

	public:
		/// Public key of the drive multisig account.
		Key Drive;

		/// Replicator public key.
		Key Replicator;

		/// Replicator deposit.
		model::UnresolvedMosaic Deposit;
	};

	struct DriveFileData {
		explicit DriveFileData(const Key& drive, const DriveFile& file)
			: DriveFileData(drive, file.Hash, file.ParentHash, file.NameSize, file.NamePtr())
		{}

		explicit DriveFileData(
				const Key& drive,
				const Hash256& hash,
				const Hash256& parentHash,
				uint8_t nameSize,
				const uint8_t* namePtr)
				: Drive(drive)
				, Hash(hash)
				, ParentHash(parentHash) {
			Name.resize(nameSize);
			memcpy(Name.data(), namePtr, nameSize);
		}

		/// Public key of the drive multisig account.
		Key Drive;

		/// Directory hash.
		Hash256 Hash;

		/// Parent directory hash.
		Hash256 ParentHash;

		/// Directory name.
		std::string Name;
	};

	/// Base class of drive file/directory notifications.
	struct FileNotification : public Notification {
		explicit FileNotification(
			NotificationType type,
			size_t size,
			const Key& drive,
			const DriveFile& file)
			: Notification(type, size)
			, File(drive, file)
		{}

		/// Drive file data.
		DriveFileData File;
	};

	/// Notification of a new drive directory.
	template<VersionType version>
	struct CreateDirectoryNotification;

	template<>
	struct CreateDirectoryNotification<1> : public FileNotification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_Create_Directory_v1_Notification;

	public:
		explicit CreateDirectoryNotification(const Key& drive, const DriveFile& file)
			: FileNotification(Notification_Type, sizeof(CreateDirectoryNotification<1>), drive, file)
		{}
	};

	/// Notification of removing drive directory.
	template<VersionType version>
	struct RemoveDirectoryNotification;

	template<>
	struct RemoveDirectoryNotification<1> : public FileNotification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_Create_Directory_v1_Notification;

	public:
		explicit RemoveDirectoryNotification(const Key& drive, const DriveFile& file)
			: FileNotification(Notification_Type, sizeof(RemoveDirectoryNotification<1>), drive, file)
		{}
	};

	/// Notification of uploading drive file.
	template<VersionType version>
	struct UploadFileNotification;

	template<>
	struct UploadFileNotification<1> : public FileNotification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_Upload_File_v1_Notification;

	public:
		explicit UploadFileNotification(const Key& drive, const DriveFile& file)
			: FileNotification(Notification_Type, sizeof(UploadFileNotification<1>), drive, file)
		{}
	};

	/// Notification of downloading drive file.
	template<VersionType version>
	struct DownloadFileNotification;

	template<>
	struct DownloadFileNotification<1> : public FileNotification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_Upload_File_v1_Notification;

	public:
		explicit DownloadFileNotification(const Key& drive, const DriveFile& file)
			: FileNotification(Notification_Type, sizeof(DownloadFileNotification<1>), drive, file)
		{}
	};

	/// Notification of drive file deletion.
	template<VersionType version>
	struct DeleteFileNotification;

	template<>
	struct DeleteFileNotification<1> : public FileNotification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_Delete_File_v1_Notification;

	public:
		explicit DeleteFileNotification(const Key& drive, const DriveFile& file)
			: FileNotification(Notification_Type, sizeof(DeleteFileNotification<1>), drive, file)
		{}
	};

	/// Base class of drive file/directory move/copy notifications.
	struct FileMoveCopyNotification : public Notification {
		explicit FileMoveCopyNotification(
			NotificationType type,
			size_t size,
			const Key& drive,
			const DriveFile& source,
			const DriveFile& destination)
			: Notification(type, size)
			, Source(drive, source)
			, Destination(drive, destination)
		{}

		/// Source drive file data.
		DriveFileData Source;

		/// Destination drive file data.
		DriveFileData Destination;
	};

	/// Notification of drive file deletion.
	template<VersionType version>
	struct MoveFileNotification;

	template<>
	struct MoveFileNotification<1> : public FileMoveCopyNotification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_Copy_File_v1_Notification;

	public:
		explicit MoveFileNotification(const Key& drive, const DriveFile& source, const DriveFile& destination)
			: FileMoveCopyNotification(Notification_Type, sizeof(MoveFileNotification<1>), drive, source, destination)
		{}
	};

	/// Notification of drive file deletion.
	template<VersionType version>
	struct CopyFileNotification;

	template<>
	struct CopyFileNotification<1> : public FileMoveCopyNotification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Service_Copy_File_v1_Notification;

	public:
		explicit CopyFileNotification(const Key& drive, const DriveFile& source, const DriveFile& destination)
			: FileMoveCopyNotification(Notification_Type, sizeof(CopyFileNotification<1>), drive, source, destination)
		{}
	};
}}
