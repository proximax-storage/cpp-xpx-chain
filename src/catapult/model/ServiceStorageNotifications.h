/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Notifications.h"
#include "catapult/state/StorageState.h"

namespace catapult { namespace model {

	/// Defines a data modification notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Data_Modification_v1, 0x0001);

	/// Defines a download notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Download_v1, 0x0002);

	/// Defines a prepare drive notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Prepare_Drive_v1, 0x0003);

	/// Defines a drive notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Drive_v1, 0x0004);

	/// Defines a data modification approval notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Data_Modification_Approval_v1, 0x0005);

	/// Defines a data modification cancel notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Data_Modification_Cancel_v1, 0x0006);

	/// Defines a replicator onboarding notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Replicator_Onboarding_v1, 0x0007);

	/// Defines a replicator offboarding notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Replicator_Offboarding_v1, 0x0008);

	/// Defines a finish download notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Finish_Download_v1, 0x0009);

	/// Defines a download payment notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Download_Payment_v1, 0x000A);

	/// Defines a storage payment notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Storage_Payment_v1, 0x000B);

	/// Defines a data modification single approval notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Data_Modification_Single_Approval_v1, 0x000C);

	/// Defines a verification payment notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Verification_Payment_v1, 0x000D);

	/// Defines an opinion notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Opinion_v1, 0x000E);

	/// Defines a download approval notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Download_Approval_v1, 0x000F);

	/// Defines a download approval payment notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Download_Approval_Payment_v1, 0x0010);

	/// Defines a download channel refund notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Download_Channel_Refund_v1, 0x0011);

	/// Defines a drive closure notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Drive_Closure_v1, 0x0012);

	/// Defines a data modification approval download work notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Data_Modification_Approval_Download_Work_v1, 0x0013);

	/// Defines a data modification approval upload work notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Data_Modification_Approval_Upload_Work_v1, 0x0014);

	/// Defines a data modification approval refund notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Data_Modification_Approval_Refund_v1, 0x0015);

	/// Defines a stream start notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Stream_Start_v1, 0x0016);

	/// Defines a stream finish notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Stream_Finish_v1, 0x0017);

	/// Defines a stream payment notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Stream_Payment_v1, 0x0018);

	/// Defines an end drive verification notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, End_Drive_Verification_v1, 0x0019);

	/// Defines an start drive verification notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Start_Drive_Verification_v1, 0x001A);

	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Owner_Management_Prohibition_v1, 0x001B);

	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Replicator_Node_Boot_Key_v1, 0x001C);

	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, ReplicatorsCleanup_v1, 0x001D);

	/// Defines a download reward notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Download_Reward_v1, 0x001E);

	/// Defines a download channel remove notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Download_Channel_Remove_v1, 0x001F);

	/// Defines a replicator onboarding notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Replicator_Onboarding_v2, 0x0020);

	/// Defines a drives update notification type.
	DEFINE_NOTIFICATION_TYPE(All, ReplicatorService, Drives_Update_v1, 0x0021);

	/// Notification about a data modification approval.
	template<VersionType version>
	struct DataModificationApprovalServiceNotification;

	template<>
	struct DataModificationApprovalServiceNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = ReplicatorService_Data_Modification_Approval_v1_Notification;

	public:
		explicit DataModificationApprovalServiceNotification(
				std::shared_ptr<state::Drive>&& pDrive,
				const Hash256& dataModificationId,
				const Hash256& fileStructureCdi,
				std::vector<Key>&& replicators)
			: Notification(Notification_Type, sizeof(DataModificationApprovalServiceNotification<1>))
			, DrivePtr(std::move(pDrive))
			, DataModificationId(dataModificationId)
			, FileStructureCdi(fileStructureCdi)
			, Replicators(std::move(replicators))
		{}

	public:
		/// Drive where data modification approved.
		std::shared_ptr<state::Drive> DrivePtr;

		/// Identifier of the transaction that initiated the modification.
		Hash256 DataModificationId;

		/// Content Download Information for the File Structure.
		Hash256 FileStructureCdi;

		/// Replicators' public keys.
		std::vector<Key> Replicators;
	};

	/// Notification about a data modification cancel.
	template<VersionType version>
	struct DataModificationCancelServiceNotification;

	template<>
	struct DataModificationCancelServiceNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = ReplicatorService_Data_Modification_Cancel_v1_Notification;

	public:
		explicit DataModificationCancelServiceNotification(
				std::shared_ptr<state::Drive>&& pDrive,
				const Hash256& dataModificationId)
			: Notification(Notification_Type, sizeof(DataModificationCancelServiceNotification<1>))
			, DrivePtr(std::move(pDrive))
			, DataModificationId(dataModificationId) {}

	public:
		/// Drive where data modification is canceled.
		std::shared_ptr<state::Drive> DrivePtr;

		/// Identifier of the transaction that initiated the modification.
		Hash256 DataModificationId;
	};

	/// Notification about a data modification.
	template<VersionType version>
	struct DataModificationServiceNotification;

	template<>
	struct DataModificationServiceNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = ReplicatorService_Data_Modification_v1_Notification;

	public:
		explicit DataModificationServiceNotification(
				std::shared_ptr<state::Drive>&& pDrive,
				const Hash256& dataModificationId,
				const Hash256& cdi,
				uint64_t uploadSize)
			: Notification(Notification_Type, sizeof(DataModificationServiceNotification<1>))
			, DrivePtr(std::move(pDrive))
			, DataModificationId(dataModificationId)
			, DownloadDataCdi(cdi)
			, UploadSizeMegabytes(uploadSize)
		{}

	public:
		/// Drive where data modification applied.
		std::shared_ptr<state::Drive> DrivePtr;

		/// Hash of the transaction.
		Hash256 DataModificationId;

		/// CDI of download data.
		Hash256 DownloadDataCdi;

		/// Upload size of data.
		uint64_t UploadSizeMegabytes;
	};

	/// Notification about a data modification single approval.
	template<VersionType version>
	struct DataModificationSingleApprovalServiceNotification;

	template<>
	struct DataModificationSingleApprovalServiceNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = ReplicatorService_Data_Modification_Single_Approval_v1_Notification;

	public:
		explicit DataModificationSingleApprovalServiceNotification(
				std::shared_ptr<state::Drive>&& pDrive,
				const Hash256& dataModificationId,
				const Key& signer)
			: Notification(Notification_Type, sizeof(DataModificationSingleApprovalServiceNotification<1>))
			, DrivePtr(std::move(pDrive))
			, DataModificationId(dataModificationId)
			, Signer(signer)
		{}

	public:
		/// Drive where data modification applied.
		std::shared_ptr<state::Drive> DrivePtr;

		/// Identifier of the transaction that initiated the modification.
		Hash256 DataModificationId;

		/// Key of the signer.
		Key Signer;
	};

	/// Notification about a download approval.
	template<VersionType version>
	struct DownloadApprovalServiceNotification;

	template<>
	struct DownloadApprovalServiceNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = ReplicatorService_Download_Approval_v1_Notification;

	public:
		explicit DownloadApprovalServiceNotification(
				std::shared_ptr<state::DownloadChannel>&& pChannel,
				bool downloadChannelClosed)
			: Notification(Notification_Type, sizeof(DownloadApprovalServiceNotification<1>))
			, DownloadChannelPtr(std::move(pChannel))
			, DownloadChannelClosed(downloadChannelClosed)
		{}

	public:
		/// Download channel.
		std::shared_ptr<state::DownloadChannel> DownloadChannelPtr;

		/// True if download channel closed, false otherwise.
		bool DownloadChannelClosed;
	};

	/// Notification about a download.
	template<VersionType version>
	struct DownloadServiceNotification;

	template<>
	struct DownloadServiceNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = ReplicatorService_Download_v1_Notification;

	public:
		explicit DownloadServiceNotification(
				std::shared_ptr<state::DownloadChannel>&& pChannel)
			: Notification(Notification_Type, sizeof(DownloadServiceNotification<1>))
			, DownloadChannelPtr(std::move(pChannel))

		{}

	public:
		/// Download channel.
		std::shared_ptr<state::DownloadChannel> DownloadChannelPtr;
	};

	/// Notification about a download payment.
	template<VersionType version>
	struct DownloadPaymentServiceNotification;

	template<>
	struct DownloadPaymentServiceNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = ReplicatorService_Download_Payment_v1_Notification;

	public:
		explicit DownloadPaymentServiceNotification(
				std::shared_ptr<state::DownloadChannel>&& pDownloadChannel)
			: Notification(Notification_Type, sizeof(DownloadPaymentServiceNotification<1>))
			, DownloadChannelPtr(std::move(pDownloadChannel))
		{}

	public:
		/// Download channel.
		std::shared_ptr<state::DownloadChannel> DownloadChannelPtr;
	};

	/// Notification about end drive verification.
	template<VersionType version>
	struct EndDriveVerificationServiceNotification;

	template<>
	struct EndDriveVerificationServiceNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = ReplicatorService_End_Drive_Verification_v1_Notification;

	public:
		explicit EndDriveVerificationServiceNotification(
				std::shared_ptr<state::Drive>&& pDrive,
				const Hash256& verificationTrigger,
				utils::KeySet&& replicators)
			: Notification(Notification_Type, sizeof(EndDriveVerificationServiceNotification<1>))
			, DrivePtr(std::move(pDrive))
			, VerificationTrigger(verificationTrigger)
			, Replicators(std::move(replicators))
			{}

	public:
		/// Drive where verification completed.
		std::shared_ptr<state::Drive> DrivePtr;

		/// The hash of block that initiated the Verification.
		Hash256 VerificationTrigger;

		/// Array of the replicator keys.
		utils::KeySet Replicators;
	};

	/// Notification about download rewards.
	template<VersionType version>
	struct DownloadRewardServiceNotification;

	template<>
	struct DownloadRewardServiceNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = ReplicatorService_Download_Reward_v1_Notification;

	public:
		explicit DownloadRewardServiceNotification(
				std::vector<std::shared_ptr<state::DownloadChannel>>&& channels)
			: Notification(Notification_Type, sizeof(DownloadRewardServiceNotification<1>))
			, Channels(std::move(channels))
		{}

	public:
		/// Download channels.
		std::vector<std::shared_ptr<state::DownloadChannel>> Channels;
	};

	/// Notification about download channel removal.
	template<VersionType version>
	struct DownloadChannelRemoveServiceNotification;

	template<>
	struct DownloadChannelRemoveServiceNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = ReplicatorService_Download_Channel_Remove_v1_Notification;

	public:
		explicit DownloadChannelRemoveServiceNotification(
				std::shared_ptr<state::DownloadChannel>&& pChannel)
			: Notification(Notification_Type, sizeof(DownloadChannelRemoveServiceNotification<1>))
			, DownloadChannelPtr(std::move(pChannel))
		{}

	public:
		/// Download channel.
		std::shared_ptr<state::DownloadChannel> DownloadChannelPtr;
	};

	/// Notification about drives' update.
	template<VersionType version>
	struct DrivesUpdateServiceNotification;

	template<>
	struct DrivesUpdateServiceNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = ReplicatorService_Drives_Update_v1_Notification;

	public:
		explicit DrivesUpdateServiceNotification(
				std::vector<std::shared_ptr<state::Drive>>&& updatedDrives,
				std::vector<Key>&& closedDrives,
				const Timestamp& timestamp)
			: Notification(Notification_Type, sizeof(DrivesUpdateServiceNotification<1>))
			, UpdatedDrives(std::move(updatedDrives))
			, ClosedDrives(std::move(closedDrives))
			, Timestamp(timestamp)
		{}

	public:
		/// Drives where replicators were added.
		std::vector<std::shared_ptr<state::Drive>> UpdatedDrives;

		/// Identifiers of closed drives.
		std::vector<Key> ClosedDrives;

		/// Block timestamp.
		catapult::Timestamp Timestamp;
	};

	/// Notification about a new drive.
	template<VersionType version>
	struct PrepareDriveServiceNotification;

	template<>
	struct PrepareDriveServiceNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = ReplicatorService_Prepare_Drive_v1_Notification;

	public:
		explicit PrepareDriveServiceNotification(
				std::shared_ptr<state::Drive>&& pDrive)
			: Notification(Notification_Type, sizeof(PrepareDriveServiceNotification<1>))
			, DrivePtr(std::move(pDrive))
		{}

	public:
		/// New drive.
		std::shared_ptr<state::Drive> DrivePtr;
	};

	/// Notification of a data modification cancel.
	template<VersionType version>
	struct ReplicatorOnboardingServiceNotification;

	template<>
	struct ReplicatorOnboardingServiceNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = ReplicatorService_Replicator_Onboarding_v1_Notification;

	public:
		explicit ReplicatorOnboardingServiceNotification(
				const Key& replicatorKey)
			: Notification(Notification_Type, sizeof(ReplicatorOnboardingServiceNotification<1>))
			, ReplicatorKey(replicatorKey)
		{}

	public:
		/// Public key of the onboarding replicator.
		Key ReplicatorKey;
	};

	/// Notification about a stream finish.
	template<VersionType version>
	struct StreamFinishServiceNotification;

	template<>
	struct StreamFinishServiceNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = ReplicatorService_Stream_Finish_v1_Notification;

	public:
		explicit StreamFinishServiceNotification(
				std::shared_ptr<state::Drive>&& pDrive,
				const Hash256& streamId,
				const Hash256& streamStructureCdi,
				const uint64_t& actualUploadSize)
			: Notification(Notification_Type, sizeof(StreamFinishServiceNotification<1>))
			, DrivePtr(std::move(pDrive))
			, StreamId(streamId)
			, StreamStructureCdi(streamStructureCdi)
			, ActualUploadSize(actualUploadSize)
		{}

	public:
		/// Drive where stream finished.
		std::shared_ptr<state::Drive> DrivePtr;

		/// Hash of the transaction.
		Hash256 StreamId;

		Hash256 StreamStructureCdi;

		/// Actual size of the stream
		uint64_t ActualUploadSize;
	};

	/// Notification of a stream payment.
	template<VersionType version>
	struct StreamPaymentServiceNotification;

	template<>
	struct StreamPaymentServiceNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = ReplicatorService_Stream_Payment_v1_Notification;

	public:
		explicit StreamPaymentServiceNotification(
				std::shared_ptr<state::Drive>&& pDrive,
				const Hash256& streamId)
			: Notification(Notification_Type, sizeof(StreamPaymentServiceNotification<1>))
			, DrivePtr(std::move(pDrive))
			, StreamId(streamId)
		{}

	public:
		/// Drive where stream has been payed for.
		std::shared_ptr<state::Drive> DrivePtr;

		/// Hash of the transaction.
		Hash256 StreamId;
	};

	/// Notification of a stream start.
	template<VersionType version>
	struct StreamStartServiceNotification;

	template<>
	struct StreamStartServiceNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = ReplicatorService_Stream_Start_v1_Notification;

	public:
		explicit StreamStartServiceNotification(
				std::shared_ptr<state::Drive>&& pDrive,
				const Hash256& streamId,
				const uint64_t& expecteedUploadSize,
				const std::string& folderName)
			: Notification(Notification_Type, sizeof(StreamStartServiceNotification<1>))
			, DrivePtr(std::move(pDrive))
			, StreamId(streamId)
			, ExpectedUploadSize(expecteedUploadSize)
			, FolderName(folderName)
		{}

	public:
		/// Drive where stream started.
		std::shared_ptr<state::Drive> DrivePtr;

		/// Hash of the transaction.
		Hash256 StreamId;

		/// Upload size of data.
		uint64_t ExpectedUploadSize;

		/// FolderName to save stream in
		std::string FolderName;
	};

	/// Notification of a stream start.
	template<VersionType version>
	struct StartDriveVerificationServiceNotification;

	template<>
	struct StartDriveVerificationServiceNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = ReplicatorService_Start_Drive_Verification_v1_Notification;

	public:
		explicit StartDriveVerificationServiceNotification(std::unordered_map<Key, std::shared_ptr<state::DriveVerification>, utils::ArrayHasher<Key>>&& verifications)
			: Notification(Notification_Type, sizeof(StartDriveVerificationServiceNotification<1>))
			, Verifications(std::move(verifications))
		{}

	public:
		/// Drives where verification started.
		std::unordered_map<Key, std::shared_ptr<state::DriveVerification>, utils::ArrayHasher<Key>> Verifications;
	};
}}