/**
*** Copyright 2025 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Notifications.h"

namespace catapult { namespace model {

	/// Defines a data modification notification type.
	DEFINE_NOTIFICATION_TYPE(All, Streaming, Stream_Start_Folder_Name_v1, 0x0001);

	/// Defines an update drive size notification type.
	DEFINE_NOTIFICATION_TYPE(All, Streaming, Update_Drive_Size_v1, 0x0002);

	/// Notification of a data modification.
	template<VersionType version>
	struct StreamStartFolderNameNotification;

	template<>
	struct StreamStartFolderNameNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Streaming_Stream_Start_Folder_Name_v1_Notification;

	public:
		explicit StreamStartFolderNameNotification(
				const uint16_t& folderNameSize)
			: Notification(Notification_Type, sizeof(StreamStartFolderNameNotification<1>))
			, FolderNameSize(folderNameSize)
		{}

	public:
		/// FolderName length.
		uint16_t FolderNameSize;
	};

	/// Notification of a drive size update.
	template<VersionType version>
	struct UpdateDriveSizeNotification;

	template<>
	struct UpdateDriveSizeNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Streaming_Update_Drive_Size_v1_Notification;

	public:
		explicit UpdateDriveSizeNotification(
				const Key& driveKey,
				const uint64_t& newDriveSize,
				const Key& driveOwner)
			: Notification(Notification_Type, sizeof(UpdateDriveSizeNotification<1>))
			, DriveKey(driveKey)
			, NewDriveSize(newDriveSize)
			, DriveOwner(driveOwner)
		{}

	public:
		/// Key of a drive to update.
		Key DriveKey;

		/// New size of the drive.
		uint64_t NewDriveSize;

		/// Public key of the drive owner.
		Key DriveOwner;
	};
}}
