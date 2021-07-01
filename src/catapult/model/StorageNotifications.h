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

	/// Defines a finish download notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Finish_Download_v1, 0x0008);

	/// Defines a download payment notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Download_Payment_v1, 0x0009);

	/// Defines a storage payment notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Storage_Payment_v1, 0x000A);

	/// Defines a data modification single approval notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Data_Modification_Single_Approval_v1, 0x000B);

	/// Defines a verification payment notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Verification_Payment_v1, 0x000C);

	struct DownloadWork : public UnresolvedAmountData {
	public:
		DownloadWork(const Key& driveKey, const Key& replicator)
				: DriveKey(driveKey)
				, Replicator(replicator)
		{}

	public:
		Key DriveKey;
		Key Replicator;
	};

	// TODO: Inherit from DownloadWork?
	struct UploadWork : public UnresolvedAmountData {
	public:
		UploadWork(const Key& driveKey, const Key& replicator, const uint8_t& opinion)
				: DriveKey(driveKey)
				, Replicator(replicator)
				, Opinion(opinion)
		{}

	public:
		Key DriveKey;
		Key Replicator;
		uint8_t Opinion;
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
			, UploadSize(uploadSize)
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
		uint64_t UploadSize;
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
			uint64_t downloadSize,
			const Amount& feedbackFeeAmount,
			const uint16_t& listOfPublicKeysSize,
			const Key* listOfPublicKeysPtr)
			: Notification(Notification_Type, sizeof(DownloadNotification<1>))
			, Id(id)
			, Consumer(consumer)
			, DownloadSize(downloadSize)
			, FeedbackFeeAmount(feedbackFeeAmount)
			, ListOfPublicKeysSize(listOfPublicKeysSize)
			, ListOfPublicKeysPtr(listOfPublicKeysPtr)

		{}

	public:
		/// Identifier of the download channel.
		Hash256 Id;

		/// Public key of the download consumer.
		Key Consumer;

		/// Delta size of download.
		uint64_t DownloadSize;

		/// Delta transaction fee in xpx.
		Amount FeedbackFeeAmount;

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
				uint64_t fileStructureSize,
				uint64_t usedDriveSize)
				: Notification(Notification_Type, sizeof(DataModificationApprovalNotification<1>))
				, DriveKey(driveKey)
				, DataModificationId(dataModificationId)
				, FileStructureCdi(fileStructureCdi)
				, FileStructureSize(fileStructureSize)
				, UsedDriveSize(usedDriveSize)
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
				const Amount& capacity)
				: Notification(Notification_Type, sizeof(ReplicatorOnboardingNotification<1>))
				, PublicKey(publicKey)
				, Capacity(capacity)
		{}

	public:
		/// Key of the replicator.
		Key PublicKey;

		/// The storage size that the replicator provides to the system.
		Amount Capacity;
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
				const Amount& feedbackFeeAmount)
				: Notification(Notification_Type, sizeof(FinishDownloadNotification<1>))
				, PublicKey(signer)
				, DownloadChannelId(downloadChannelId)
				, FeedbackFeeAmount(feedbackFeeAmount)
		{}

	public:
		/// Key of the signer.
		Key PublicKey;

		/// The identifier of the download channel.
		Hash256 DownloadChannelId;

		/// Amount of XPXs to transfer to the download channel.
		Amount FeedbackFeeAmount;
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
				const uint64_t downloadSize,
				const Amount& feedbackFeeAmount)
				: Notification(Notification_Type, sizeof(DownloadPaymentNotification<1>))
				, PublicKey(signer)
				, DownloadChannelId(downloadChannelId)
				, DownloadSize(downloadSize)
				, FeedbackFeeAmount(feedbackFeeAmount)
		{}

	public:
		/// Key of the signer.
		Key PublicKey;

		/// The identifier of the download channel.
		Hash256 DownloadChannelId;

		/// Download size to add to the prepaid size of the download channel.
		uint64_t DownloadSize;

		/// Amount of XPXs to transfer to the download channel.
		Amount FeedbackFeeAmount;
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
				const Key& driveKey,
				const Amount& storageUnits)
				: Notification(Notification_Type, sizeof(StoragePaymentNotification<1>))
				, PublicKey(signer)
				, DriveKey(driveKey)
				, StorageUnits(storageUnits)
		{}

	public:
		/// Key of the signer.
		Key PublicKey;

		/// Key of the drive.
		Key DriveKey;

		/// Amount of storage units to transfer to the drive.
		Amount StorageUnits;
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
				const Key& driveKey,
				const Hash256& dataModificationId,
				const uint16_t uploadOpinionPairCount,
				const Key* uploaderKeysPtr,
				const uint8_t* uploadOpinionPtr)
				: Notification(Notification_Type, sizeof(DataModificationSingleApprovalNotification<1>))
				, DriveKey(driveKey)
				, DataModificationId(dataModificationId)
				, UploadOpinionPairCount(uploadOpinionPairCount)
				, UploaderKeysPtr(uploaderKeysPtr)
				, UploadOpinionPtr(uploadOpinionPtr)
		{}

	public:
		/// Key of drive.
		Key DriveKey;

		/// Identifier of the transaction that initiated the modification.
		Hash256 DataModificationId;

		/// Number of key-opinion pairs in the payload.
		uint16_t UploadOpinionPairCount;

		/// List of the Uploader keys (current Replicators of the Drive or the Drive Owner).
		const Key* UploaderKeysPtr;

		/// Opinion about how much each Uploader has uploaded to the signer in percents.
		const uint8_t* UploadOpinionPtr;
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
				const Key& driveKey,
				const Amount& verificationFeeAmount)
				: Notification(Notification_Type, sizeof(VerificationPaymentNotification<1>))
				, Owner(owner)
				, DriveKey(driveKey)
				, VerificationFeeAmount(verificationFeeAmount)
		{}

	public:
		/// Public key of the drive owner.
		Key Owner;

		/// Key of drive.
		Key DriveKey;

		/// Amount of XPXs to transfer to the drive.
		Amount VerificationFeeAmount;
	};
}}
