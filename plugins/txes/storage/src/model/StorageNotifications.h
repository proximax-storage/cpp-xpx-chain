/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Notifications.h"
#include "catapult/utils/MemoryUtils.h"
#include <vector>

namespace catapult { namespace model {

	// TODO: Reorder codes?

	/// Defines a data modification notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Data_Modification_v1, 0x0001);

	/// Defines a download notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Download_v1, 0x0002);

	/// Defines a prepare drive notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Prepare_Drive_v1, 0x0003);

	/// Defines a drive notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Drive_v1, 0x0004);

	/// Defines a data modification cancel notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Data_Modification_Cancel_v1, 0x0005);

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
			const Key& drive,
			const Key& owner,
            const Hash256& cdi,
			const uint64_t& uploadSize)
			: Notification(Notification_Type, sizeof(DataModificationNotification<1>))
			, DriveKey(drive)
			, Owner(owner)
			, DownloadDataCDI(cdi)
			, UploadSize(uploadSize)
		{}

	public:
		/// Public key of the drive multisig account.
		Key DriveKey;

		/// Public key of the drive owner.
		Key Owner;

		/// CDI of download data.
		Hash256 DownloadDataCDI;

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
			const Key& drive,
			const Key& consumer,
			const Amount& deltaSize,
			const Amount& deltaFee)
			: Notification(Notification_Type, sizeof(DownloadNotification<1>))
			, Id(id)
			, DriveKey(drive)
			, Consumer(consumer)
			, DownloadSize(deltaSize)
		{}

	public:
		/// Identifier of the download channel.
		Hash256 Id;

		/// Public key of the drive multisig account.
		Key DriveKey;

		/// Public key of the download consumer.
		Key Consumer;

		/// Delta size of download.
		Amount DownloadSize;

		/// Delta transaction fee in xpx.
		Amount TransactionFee;
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
			const Amount& driveSize,
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
		Amount DriveSize;

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

	/// Notification of a data modification cancel.
	template<VersionType version>
	struct DataModificationCancelNotification;

	template<>
	struct DataModificationCancelNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Storage_Data_Modification_Cancel_v1_Notification;

	public:
		explicit DataModificationCancelNotification(const Key& drive, const Key& owner, const Hash256& modificationTrx)
			: Notification(Notification_Type, sizeof(DataModificationCancelNotification<1>))
			, DriveKey(drive)
			, Owner(owner)
			, ModificationTrx(modificationTrx) {}

	public:
		/// Public key of a drive multisig account.
		Key DriveKey;

		/// Public key of a drive owner.
		Key Owner;

		/// Hash of a DataModification transaction
		Hash256 ModificationTrx;
	};
}}
