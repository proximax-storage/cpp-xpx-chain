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

	/// Defines a data modification notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Data_Modification_v1, 0x0001);

	/// Defines a download notification type.
	DEFINE_NOTIFICATION_TYPE(All, Storage, Download_v1, 0x0002);

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
}}
