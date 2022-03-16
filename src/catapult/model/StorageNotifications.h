/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "catapult/model/Notifications.h"

namespace catapult { namespace model {

	/// Defines a data modification notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Data_Modification_v1, 0x0001);

	/// Defines a download notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Download_v1, 0x0002);

	/// Defines a prepare drive notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Prepare_Drive_v1, 0x0003);

	/// Defines a drive notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Drive_v1, 0x0004);

	/// Defines a data modification approval notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Data_Modification_Approval_v1, 0x0005);

	/// Defines a data modification cancel notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Data_Modification_Cancel_v1, 0x0006);

	/// Defines a replicator onboarding notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Replicator_Onboarding_v1, 0x0007);

	/// Defines a replicator offboarding notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Replicator_Offboarding_v1, 0x0008);

	/// Defines a finish download notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Finish_Download_v1, 0x0009);

	/// Defines a download payment notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Download_Payment_v1, 0x000A);

	/// Defines a storage payment notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Storage_Payment_v1, 0x000B);

	/// Defines a data modification single approval notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Data_Modification_Single_Approval_v1, 0x000C);

	/// Defines a verification payment notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Verification_Payment_v1, 0x000D);

	/// Defines an opinion notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Opinion_v1, 0x000E);

	/// Defines a download approval notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Download_Approval_v1, 0x000F);

	/// Defines a download approval payment notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Download_Approval_Payment_v1, 0x0010);

	/// Defines a download channel refund notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Download_Channel_Refund_v1, 0x0011);

	/// Defines a drive closure notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Drive_Closure_v1, 0x0012);

	/// Defines a data modification approval download work notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Data_Modification_Approval_Download_Work_v1, 0x0013);

	/// Defines a data modification approval upload work notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Data_Modification_Approval_Upload_Work_v1, 0x0014);

	/// Defines a data modification approval refund notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Data_Modification_Approval_Refund_v1, 0x0015);

	/// Defines a stream start notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Stream_Start_v1, 0x0016);

	/// Defines a stream finish notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Stream_Finish_v1, 0x0017);

	/// Defines a stream payment notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Stream_Payment_v1, 0x0018);

	/// Defines an end drive verification notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, End_Drive_Verification_v1, 0x0019);

	/// Defines an start drive verification notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Start_Drive_Verification_v1, 0x001A);

	struct DownloadPayment : public UnresolvedAmountData {
	public:
		DownloadPayment(const Hash256& downloadChannelId, const uint64_t& downloadSize)
			: DownloadChannelId(downloadChannelId)
			, DownloadSize(downloadSize)
		{}

	public:
		Hash256 DownloadChannelId;
		uint64_t DownloadSize;
	};

	struct StreamingWork : public UnresolvedAmountData {
	public:
		StreamingWork(const Key& driveKey, const uint64_t& uploadSize)
			: DriveKey(driveKey)
			, UploadSize(uploadSize)
		{}

	public:
		Key DriveKey;
		uint64_t UploadSize;
	};

	/// Notification of a data modification.
	template<VersionType version>
	struct DataModificationNotification;

	template<>
	struct DataModificationNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_Data_Modification_v1_Notification;

	public:
		explicit DataModificationNotification(
				const Hash256& dataModificationId,
				const Key& drive,
				const Key& owner,
				const Hash256& cdi,
				const uint64_t& uploadSize)
			: Notification(Notification_Type, sizeof(DataModificationNotification<1>))
			, DataModificationId(dataModificationId)
			, DriveKey(drive)
			, Owner(owner)
			, DownloadDataCdi(cdi)
			, UploadSizeMegabytes(uploadSize)
		{}

	public:
		/// Hash of the transaction.
		Hash256 DataModificationId;

		/// Public key of the drive multisig account.
		Key DriveKey;

		/// Public key of the drive owner.
		Key Owner;

		/// CDI of download data.
		Hash256 DownloadDataCdi;

		/// Upload size of data.
		uint64_t UploadSizeMegabytes;
	};

	/// Notification of a stream start.
	template<VersionType version>
	struct StreamStartNotification;

	template<>
	struct StreamStartNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_Stream_Start_v1_Notification;

	public:
		explicit StreamStartNotification(
				const Hash256& streamId,
				const Key& drive,
				const Key& owner,
				const uint64_t& expecteedUploadSize,
				const std::string& folderName)
			: Notification(Notification_Type, sizeof(StreamStartNotification<1>))
			, StreamId(streamId)
			, DriveKey(drive)
			, Owner(owner)
			, ExpectedUploadSize(expecteedUploadSize)
			, FolderName(folderName)
		{}

	public:
		/// Hash of the transaction.
		Hash256 StreamId;

		/// Public key of the drive multisig account.
		Key DriveKey;

		/// Public key of the drive owner.
		Key Owner;

		/// Upload size of data.
		uint64_t ExpectedUploadSize;

		/// FolderName to save stream in
		std::string FolderName;
	};

	/// Notification of a stream finish.
	template<VersionType version>
	struct StreamFinishNotification;

	template<>
	struct StreamFinishNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_Stream_Finish_v1_Notification;

	public:
		explicit StreamFinishNotification(
				const Key& drive,
				const Hash256& streamId,
				const Key& owner,
				const uint64_t& actualUploadSize,
				const Hash256& streamStructureCdi)
				: Notification(Notification_Type, sizeof(StreamStartNotification<1>))
				, StreamId(streamId)
				, DriveKey(drive)
				, Owner(owner)
				, ActualUploadSize(actualUploadSize)
				, StreamStructureCdi(streamStructureCdi)
				{}

	public:
		/// Hash of the transaction.
		Hash256 StreamId;

		/// Public key of the drive multisig account.
		Key DriveKey;

		/// Public key of the drive owner.
		Key Owner;

		/// Actual size of the stream
		uint64_t ActualUploadSize;

		Hash256 StreamStructureCdi;
	};

	/// Notification of a stream payment.
	template<VersionType version>
	struct StreamPaymentNotification;

	template<>
	struct StreamPaymentNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_Stream_Payment_v1_Notification;

	public:
		explicit StreamPaymentNotification(
				const Key& drive,
				const Hash256& streamId,
				const uint64_t& additionalUploadSize)
				: Notification(Notification_Type, sizeof(StreamStartNotification<1>))
				, StreamId(streamId)
				, DriveKey(drive)
				, AdditionalUploadSize(additionalUploadSize)
				{}

	public:
		/// Hash of the transaction.
		Hash256 StreamId;

		/// Public key of the drive multisig account.
		Key DriveKey;

		/// Actual size of the stream
		uint64_t AdditionalUploadSize;
	};

	/// Notification of a download.
	template<VersionType version>
	struct DownloadNotification;

	template<>
	struct DownloadNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_Download_v1_Notification;

	public:
		explicit DownloadNotification(
				const Hash256& id,
				const Key& consumer,
				const Key& drive,
				uint64_t downloadSizeMegabytes,
				uint16_t listOfPublicKeysSize,
				const Key* listOfPublicKeysPtr)
			: Notification(Notification_Type, sizeof(DownloadNotification<1>))
			, Id(id)
			, Consumer(consumer)
			, DriveKey(drive)
			, DownloadSizeMegabytes(downloadSizeMegabytes)
			, ListOfPublicKeysSize(listOfPublicKeysSize)
			, ListOfPublicKeysPtr(listOfPublicKeysPtr)

		{}

	public:
		/// Identifier of the download channel.
		Hash256 Id;

		/// Public key of the download consumer.
		Key Consumer;

		/// Public key of the drive.
		Key DriveKey;

		/// Delta size of download.
		uint64_t DownloadSizeMegabytes;

		/// Size of the list of public keys
		uint16_t ListOfPublicKeysSize;

		/// List of public keys.
		const Key* ListOfPublicKeysPtr;
	};

	/// Notification of a drive preparation.
	template<VersionType version>
	struct PrepareDriveNotification;

	template<>
	struct PrepareDriveNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_Prepare_Drive_v1_Notification;

	public:
		explicit PrepareDriveNotification(
				const Key& owner,
				const Key& driveKey,
				const uint64_t& driveSize,
				const uint16_t& replicatorCount)
			: Notification(Notification_Type, sizeof(PrepareDriveNotification<1>))
			, Owner(owner)
			, DriveKey(driveKey)
			, DriveSize(driveSize)
			, ReplicatorCount(replicatorCount)
		{}

	public:
		/// Public key of owner.
		Key Owner;

		/// Public key of drive.
		Key DriveKey;

		/// Size of drive.
		uint64_t DriveSize;

		/// Number of replicators.
		uint16_t ReplicatorCount;
	};

	/// Notification of a drive.
	template<VersionType version>
	struct DriveNotification;

	template<>
	struct DriveNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_Drive_v1_Notification;

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

	/// Notification of a data modification approval.
	template<VersionType version>
	struct DataModificationApprovalNotification;

	template<>
	struct DataModificationApprovalNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_Data_Modification_Approval_v1_Notification;

	public:
		explicit DataModificationApprovalNotification(
				const Key& driveKey,
				const Hash256& dataModificationId,
				const Hash256& fileStructureCdi,
				const uint64_t fileStructureSize,
				const uint64_t metaFilesSize,
				const uint64_t usedDriveSize,
				const uint8_t judgingKeysCount,
				const uint8_t overlappingKeysCount,
				const uint8_t judgedKeysCount,
				const Key* publicKeysPtr,
				const uint8_t* presentOpinionsPtr)
			: Notification(Notification_Type, sizeof(DataModificationApprovalNotification<1>))
			, DriveKey(driveKey)
			, DataModificationId(dataModificationId)
			, FileStructureCdi(fileStructureCdi)
			, FileStructureSize(fileStructureSize)
			, MetaFilesSize(metaFilesSize)
			, UsedDriveSize(usedDriveSize)
			, JudgingKeysCount(judgingKeysCount)
			, OverlappingKeysCount(overlappingKeysCount)
			, JudgedKeysCount(judgedKeysCount)
			, PublicKeysPtr(publicKeysPtr)
			, PresentOpinionsPtr(presentOpinionsPtr)
		{}

	public:
		/// Key of drive.
		Key DriveKey;

		/// Identifier of the transaction that initiated the modification.
		Hash256 DataModificationId;

		/// Content Download Information for the File Structure.
		Hash256 FileStructureCdi;

		/// Size of the File Structure.
		uint64_t FileStructureSize;

		/// The size of metafiles including File Structure.
		uint64_t MetaFilesSize;

		/// Total used disk space of the drive.
		uint64_t UsedDriveSize;

		/// Number of replicators that provided their opinions, but on which no opinions were provided.
		uint8_t JudgingKeysCount;

		/// Number of replicators that both provided their opinions, and on which at least one opinion was provided.
		uint8_t OverlappingKeysCount;

		/// Number of replicators that didn't provide their opinions, but on which at least one opinion was provided.
		uint8_t JudgedKeysCount;

		/// Replicators' public keys.
		const Key* PublicKeysPtr;

		/// Two-dimensional array of opinion element presence.
		/// Must be interpreted bitwise (1 if corresponding element exists, 0 otherwise).
		const uint8_t* PresentOpinionsPtr;
	};

	/// Notification of a data modification approval download work.
	template<VersionType version>
	struct DataModificationApprovalDownloadWorkNotification;

	template<>
	struct DataModificationApprovalDownloadWorkNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_Data_Modification_Approval_Download_Work_v1_Notification;

	public:
		explicit DataModificationApprovalDownloadWorkNotification(
				const Key& driveKey,
				const Hash256& dataModificationId,
				const uint8_t publicKeysCount,
				const Key* publicKeysPtr)
				: Notification(Notification_Type, sizeof(DataModificationApprovalNotification<1>))
				, DriveKey(driveKey)
				, DataModificationId(dataModificationId)
				, PublicKeysCount(publicKeysCount)
				, PublicKeysPtr(publicKeysPtr)
		{}

	public:
		/// Key of drive.
		Key DriveKey;

		/// Identifier of the transaction that initiated the modification.
		Hash256 DataModificationId;

		/// Number of replicators' public keys.
		uint8_t PublicKeysCount;

		/// Replicators' public keys.
		const Key* PublicKeysPtr;
	};

	/// Notification of a data modification approval upload work.
	template<VersionType version>
	struct DataModificationApprovalUploadWorkNotification;

	template<>
	struct DataModificationApprovalUploadWorkNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_Data_Modification_Approval_Upload_Work_v1_Notification;

	public:
		explicit DataModificationApprovalUploadWorkNotification(
				const Key& driveKey,
				const Hash256& modificationId,
				const uint8_t judgingKeysCount,
				const uint8_t overlappingKeysCount,
				const uint8_t judgedKeysCount,
				const Key* publicKeysPtr,
				const uint8_t* presentOpinionsPtr,
				const uint64_t* opinionsPtr)
				: Notification(Notification_Type, sizeof(DataModificationApprovalNotification<1>))
				, DriveKey(driveKey)
				, ModificationId(modificationId)
				, JudgingKeysCount(judgingKeysCount)
				, OverlappingKeysCount(overlappingKeysCount)
				, JudgedKeysCount(judgedKeysCount)
				, PublicKeysPtr(publicKeysPtr)
				, PresentOpinionsPtr(presentOpinionsPtr)
				, OpinionsPtr(opinionsPtr)
		{}

	public:
		/// Key of drive.
		Key DriveKey;

		/// Id of the approved modification
		Hash256 ModificationId;

		/// Number of replicators that provided their opinions, but on which no opinions were provided.
		uint8_t JudgingKeysCount;

		/// Number of replicators that both provided their opinions, and on which at least one opinion was provided.
		uint8_t OverlappingKeysCount;

		/// Number of replicators that didn't provide their opinions, but on which at least one opinion was provided.
		uint8_t JudgedKeysCount;

		/// Replicators' public keys.
		const Key* PublicKeysPtr;

		/// Two-dimensional array of opinion element presence.
		/// Must be interpreted bitwise (1 if corresponding element exists, 0 otherwise).
		const uint8_t* PresentOpinionsPtr;

		/// Jagged array of opinion elements.
		const uint64_t* OpinionsPtr;
	};

	/// Notification of a data modification approval refund.
	template<VersionType version>
	struct DataModificationApprovalRefundNotification;

	template<>
	struct DataModificationApprovalRefundNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_Data_Modification_Approval_Refund_v1_Notification;

	public:
		explicit DataModificationApprovalRefundNotification(
				const Key& driveKey,
				const Hash256& dataModificationId,
				const uint64_t metaFilesSizeBytes,
				const uint64_t usedDriveSizeBytes)
				: Notification(Notification_Type, sizeof(DataModificationApprovalNotification<1>))
				, DriveKey(driveKey)
				, DataModificationId(dataModificationId)
				, MetaFilesSizeBytes(metaFilesSizeBytes)
				, UsedDriveSize(usedDriveSizeBytes)
		{}

	public:
		/// Key of drive.
		Key DriveKey;

		/// Identifier of the transaction that initiated the modification.
		Hash256 DataModificationId;

		/// The size of metafiles including File Structure.
		uint64_t MetaFilesSizeBytes;

		/// Total used disk space of the drive.
		uint64_t UsedDriveSize;
	};

	/// Notification of a data modification cancel.
	template<VersionType version>
	struct DataModificationCancelNotification;

	template<>
	struct DataModificationCancelNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_Data_Modification_Cancel_v1_Notification;

	public:
		explicit DataModificationCancelNotification(const Key& drive, const Key& owner, const Hash256& dataModificationId)
			: Notification(Notification_Type, sizeof(DataModificationCancelNotification<1>))
			, DriveKey(drive)
			, Owner(owner)
			, DataModificationId(dataModificationId) {}

	public:
		/// Public key of a drive multisig account.
		Key DriveKey;

		/// Public key of a drive owner.
		Key Owner;

		/// Identifier of the transaction that initiated the modification.
		Hash256 DataModificationId;
	};

	/// Notification of a replicator onboarding.
	template<VersionType version>
	struct ReplicatorOnboardingNotification;

	template<>
	struct ReplicatorOnboardingNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_Replicator_Onboarding_v1_Notification;

	public:
		explicit ReplicatorOnboardingNotification(
				const Key& publicKey,
				const Amount& capacity,
				const Hash256& seed)
			: Notification(Notification_Type, sizeof(ReplicatorOnboardingNotification<1>))
			, PublicKey(publicKey)
			, Capacity(capacity)
			, Seed(seed)
		{}

	public:
		/// Key of the replicator.
		Key PublicKey;

		/// The storage size that the replicator provides to the system.
		Amount Capacity;

		/// Seed that is used for random number generator.
		Hash256 Seed;
	};

	/// Notification of a drive closure.
	template<VersionType version>
	struct DriveClosureNotification;

	template<>
	struct DriveClosureNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_Drive_Closure_v1_Notification;

	public:
		explicit DriveClosureNotification(const Hash256& transactionHash, const Key& drive, const Key& owner)
			: Notification(Notification_Type, sizeof(DriveClosureNotification<1>))
			, TransactionHash(transactionHash)
			, DriveKey(drive)
			, DriveOwner(owner)
		{}

	public:
		/// Hash of the drive closure transaction.
		Hash256 TransactionHash;

		/// Public key of a drive.
		Key DriveKey;

		/// Public Key of the drive owner
		Key DriveOwner;
	};

	/// Notification of a replicator offboarding.
	template<VersionType version>
	struct ReplicatorOffboardingNotification;

	template<>
	struct ReplicatorOffboardingNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_Replicator_Offboarding_v1_Notification;

	public:
		explicit ReplicatorOffboardingNotification(const Key& publicKey, const Key& drive)
			: Notification(Notification_Type, sizeof(ReplicatorOffboardingNotification<1>))
			, PublicKey(publicKey)
			, DriveKey(drive)
		{}

	public:
		/// Key of the replicator.
		Key PublicKey;

		/// Public key of a drive.
		Key DriveKey;

	};

	/// Notification of a finish download.
	template<VersionType version>
	struct FinishDownloadNotification;

	template<>
	struct FinishDownloadNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_Finish_Download_v1_Notification;

	public:
		explicit FinishDownloadNotification(
				const Key& signer,
				const Hash256& downloadChannelId,
				const Hash256& transactionHash)
			: Notification(Notification_Type, sizeof(FinishDownloadNotification<1>))
			, PublicKey(signer)
			, DownloadChannelId(downloadChannelId)
			, TransactionHash(transactionHash)
		{}

	public:
		/// Key of the signer.
		Key PublicKey;

		/// The identifier of the download channel.
		Hash256 DownloadChannelId;

		/// The hash of the transaction
		Hash256 TransactionHash;
	};

	/// Notification of a download payment.
	template<VersionType version>
	struct DownloadPaymentNotification;

	template<>
	struct DownloadPaymentNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_Download_Payment_v1_Notification;

	public:
		explicit DownloadPaymentNotification(
				const Key& signer,
				const Hash256& downloadChannelId,
				const uint64_t downloadSizeMegabytes)
			: Notification(Notification_Type, sizeof(DownloadPaymentNotification<1>))
			, PublicKey(signer)
			, DownloadChannelId(downloadChannelId)
			, DownloadSizeMegabytes(downloadSizeMegabytes)
		{}

	public:
		/// Key of the signer.
		Key PublicKey;

		/// The identifier of the download channel.
		Hash256 DownloadChannelId;

		/// Download size to add to the prepaid size of the download channel.
		uint64_t DownloadSizeMegabytes;
	};

	/// Notification of a storage payment.
	template<VersionType version>
	struct StoragePaymentNotification;

	template<>
	struct StoragePaymentNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_Storage_Payment_v1_Notification;

	public:
		explicit StoragePaymentNotification(
				const Key& signer,
				const Key& driveKey)
			: Notification(Notification_Type, sizeof(StoragePaymentNotification<1>))
			, PublicKey(signer)
			, DriveKey(driveKey)
		{}

	public:
		/// Key of the signer.
		Key PublicKey;

		/// Key of the drive.
		Key DriveKey;
	};

	/// Notification of a data modification single approval.
	template<VersionType version>
	struct DataModificationSingleApprovalNotification;

	template<>
	struct DataModificationSingleApprovalNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_Data_Modification_Single_Approval_v1_Notification;

	public:
		explicit DataModificationSingleApprovalNotification(
				const Key& signer,
				const Key& driveKey,
				const Hash256& dataModificationId,
				const uint16_t publicKeysCount,
				const Key* publicKeysPtr,
				const uint64_t* opinionsPtr)
			: Notification(Notification_Type, sizeof(DataModificationSingleApprovalNotification<1>))
			, PublicKey(signer)
			, DriveKey(driveKey)
			, DataModificationId(dataModificationId)
			, PublicKeysCount(publicKeysCount)
			, PublicKeysPtr(publicKeysPtr)
			, OpinionsPtr(opinionsPtr)
		{}

	public:
		/// Key of the signer.
		Key PublicKey;

		/// Key of drive.
		Key DriveKey;

		/// Identifier of the transaction that initiated the modification.
		Hash256 DataModificationId;

		/// Number of replicators' public keys.
		uint8_t PublicKeysCount;

		/// Replicators' public keys.
		const Key* PublicKeysPtr;

		/// One-dimensional array of opinion elements.
		const uint64_t* OpinionsPtr;
	};

	/// Notification of a verification payment.
	template<VersionType version>
	struct VerificationPaymentNotification;

	template<>
	struct VerificationPaymentNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_Verification_Payment_v1_Notification;

	public:
		explicit VerificationPaymentNotification(
				const Key& owner,
				const Key& driveKey)
			: Notification(Notification_Type, sizeof(VerificationPaymentNotification<1>))
			, Owner(owner)
			, DriveKey(driveKey)
		{}

	public:
		/// Public key of the drive owner.
		Key Owner;

		/// Key of drive.
		Key DriveKey;
	};

	/// Notification of an opinion multisig.
	template<VersionType version> //, typename TOpinion>
	struct OpinionNotification;

	template<> //typename TOpinion>
	struct OpinionNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_Opinion_v1_Notification;

	public:
		explicit OpinionNotification(
				const size_t commonDataSize,
				const uint8_t judgingKeysCount,
				const uint8_t overlappingKeysCount,
				const uint8_t judgedKeysCount,
				const uint8_t opinionElementSize,
				const uint8_t* commonDataPtr,
				const Key* publicKeysPtr,
				const Signature* signaturesPtr,
				const uint8_t* presentOpinionsPtr,
				const uint8_t* opinionsPtr)
			: Notification(Notification_Type, sizeof(OpinionNotification<1>))
			, CommonDataSize(commonDataSize)
			, JudgingKeysCount(judgingKeysCount)
			, OverlappingKeysCount(overlappingKeysCount)
			, JudgedKeysCount(judgedKeysCount)
			, OpinionElementSize(opinionElementSize)
			, CommonDataPtr(commonDataPtr)
			, PublicKeysPtr(publicKeysPtr)
			, SignaturesPtr(signaturesPtr)
			, PresentOpinionsPtr(presentOpinionsPtr)
			, OpinionsPtr(opinionsPtr)
		{}

	public:
		/// Size of common data of the transaction in bytes.
		size_t CommonDataSize;

		/// Number of replicators that provided their opinions, but on which no opinions were provided.
		uint8_t JudgingKeysCount;

		/// Number of replicators that both provided their opinions, and on which at least one opinion was provided.
		uint8_t OverlappingKeysCount;

		/// Number of replicators that didn't provide their opinions, but on which at least one opinion was provided.
		uint8_t JudgedKeysCount;

		/// Size of one opinion element in bytes.
		uint8_t OpinionElementSize;

		/// Common data of the transaction.
		const uint8_t* CommonDataPtr;

		/// Replicators' public keys.
		const Key* PublicKeysPtr;

		/// Signatures of replicators' opinions.
		const Signature* SignaturesPtr;

		/// Two-dimensional array of opinion element presence.
		/// Must be interpreted bitwise (1 if corresponding element exists, 0 otherwise).
		const uint8_t* PresentOpinionsPtr;

		/// Pointer to the beginning of jagged array of opinion elements.
		const uint8_t* OpinionsPtr;
	};

	/// Notification of a download approval.
	template<VersionType version>
	struct DownloadApprovalNotification;

	template<>
	struct DownloadApprovalNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_Download_Approval_v1_Notification;

	public:
		explicit DownloadApprovalNotification(
				const Hash256& id,
				const Hash256& approvalTrigger,
				const uint8_t judgingKeysCount,
				const uint8_t overlappingKeysCount,
				const uint8_t judgedKeysCount,
				const Key* publicKeysPtr,
				const uint8_t* presentOpinionsPtr)
			: Notification(Notification_Type, sizeof(DownloadApprovalNotification<1>))
			, DownloadChannelId(id)
			, ApprovalTrigger(approvalTrigger)
			, JudgingKeysCount(judgingKeysCount)
			, OverlappingKeysCount(overlappingKeysCount)
			, JudgedKeysCount(judgedKeysCount)
			, PublicKeysPtr(publicKeysPtr)
			, PresentOpinionsPtr(presentOpinionsPtr)
		{}

	public:
		/// The identifier of the download channel.
		Hash256 DownloadChannelId;

		/// The hash of the block that initiated the rewards approval.
		Hash256 ApprovalTrigger;

		/// Number of replicators that provided their opinions, but on which no opinions were provided.
		uint8_t JudgingKeysCount;

		/// Number of replicators that both provided their opinions, and on which at least one opinion was provided.
		uint8_t OverlappingKeysCount;

		/// Number of replicators that didn't provide their opinions, but on which at least one opinion was provided.
		uint8_t JudgedKeysCount;

		/// Replicators' public keys.
		const Key* PublicKeysPtr;

		/// Two-dimensional array of opinion element presence.
		/// Must be interpreted bitwise (1 if corresponding element exists, 0 otherwise).
		const uint8_t* PresentOpinionsPtr;
	};

	/// Notification of an opinion-based payment for a download approval transaction.
	template<VersionType version>
	struct DownloadApprovalPaymentNotification;

	template<>
	struct DownloadApprovalPaymentNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_Download_Approval_Payment_v1_Notification;

	public:
		explicit DownloadApprovalPaymentNotification(
				const Hash256& id,
				const uint8_t judgingKeysCount,
				const uint8_t overlappingKeysCount,
				const uint8_t judgedKeysCount,
				const Key* publicKeysPtr,
				const uint8_t* presentOpinionsPtr,
				const uint64_t* opinionsPtr)
			: Notification(Notification_Type, sizeof(DownloadApprovalPaymentNotification<1>))
			, DownloadChannelId(id)
			, JudgingKeysCount(judgingKeysCount)
			, OverlappingKeysCount(overlappingKeysCount)
			, JudgedKeysCount(judgedKeysCount)
			, PublicKeysPtr(publicKeysPtr)
			, PresentOpinionsPtr(presentOpinionsPtr)
			, OpinionsPtr(opinionsPtr)
		{}

	public:
		/// The identifier of the download channel.
		Hash256 DownloadChannelId;

		/// Number of replicators that provided their opinions, but on which no opinions were provided.
		uint8_t JudgingKeysCount;

		/// Number of replicators that both provided their opinions, and on which at least one opinion was provided.
		uint8_t OverlappingKeysCount;

		/// Number of replicators that didn't provide their opinions, but on which at least one opinion was provided.
		uint8_t JudgedKeysCount;

		/// Replicators' public keys.
		const Key* PublicKeysPtr;

		/// Two-dimensional array of opinion element presence.
		/// Must be interpreted bitwise (1 if corresponding element exists, 0 otherwise).
		const uint8_t* PresentOpinionsPtr;

		/// Jagged array of opinion elements.
		const uint64_t* OpinionsPtr;
	};

	/// Notification of a download channel refund.
	template<VersionType version>
	struct DownloadChannelRefundNotification;

	template<>
	struct DownloadChannelRefundNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_Download_Channel_Refund_v1_Notification;

	public:
		explicit DownloadChannelRefundNotification(
				const Hash256& downloadChannelId)
			: Notification(Notification_Type, sizeof(DownloadChannelRefundNotification<1>))
			, DownloadChannelId(downloadChannelId)
		{}

	public:
		/// The identifier of the download channel.
		Hash256 DownloadChannelId;
	};

	/// Notification of end drive verification.
	template<VersionType version>
	struct EndDriveVerificationNotification;

	template<>
	struct EndDriveVerificationNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_End_Drive_Verification_v1_Notification;

	public:
		explicit EndDriveVerificationNotification(
				const Key& driveKey,
				const Hash256& verificationTrigger,
				const uint16_t shardId,
				const uint16_t keyCount,
				const uint16_t judgingKeyCount,
				const Key* pPublicKeys,
				const Signature* pSignatures,
				const uint8_t* pOpinions)
				: Notification(Notification_Type, sizeof(EndDriveVerificationNotification<1>))
				, DriveKey(driveKey)
				, VerificationTrigger(verificationTrigger)
				, ShardId(shardId)
				, KeyCount(keyCount)
				, JudgingKeyCount(judgingKeyCount)
				, PublicKeysPtr(pPublicKeys)
				, SignaturesPtr(pSignatures)
				, OpinionsPtr(pOpinions)
				{}

	public:
		/// Key of the drive.
		Key DriveKey;

		/// The hash of block that initiated the Verification.
		Hash256 VerificationTrigger;

		/// Shard identifier.
		uint16_t ShardId;

		/// Number of replicators.
		uint16_t KeyCount;

		/// Number of replicators that provided their opinions.
		uint8_t JudgingKeyCount;

		/// Array of the replicator keys.
		const Key* PublicKeysPtr;

		/// Array or signatures.
		const Signature* SignaturesPtr;

		/// Array or signatures.
		const uint8_t* OpinionsPtr;
	};
}}
