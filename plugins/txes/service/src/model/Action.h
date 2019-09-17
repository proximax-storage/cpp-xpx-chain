/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Mosaic.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Cosignatory modification type.
	enum class DriveActionType : uint8_t {
		/// Prepare drive.
		PrepareDrive,

		/// Drive prolongation.
		DriveProlongation,

		/// Drive deposit.
		DriveDeposit,

		/// Drive deposit return.
		DriveDepositReturn,

		/// Drive payment.
		DrivePayment,

		/// File deposit.
		FileDeposit,

		/// File deposit return.
		FileDepositReturn,

		/// File payment.
		FilePayment,

		/// Drive verification.
		DriveVerification,

		/// Create directory.
		CreateDirectory,

		/// Remove directory.
		RemoveDirectory,

		/// Upload file.
		UploadFile,

		/// Download file.
		DownloadFile,

		/// Delete file.
		DeleteFile,

		/// Move file.
		MoveFile,

		/// Copy file.
		CopyFile,
	};

	struct ActionPrepareDrive {
		BlockDuration Duration;
		uint64_t Size;
		uint16_t Replicas;
	};

	struct ActionDriveProlongation {
		BlockDuration Duration;
	};

	struct ActionFileDeposit {
		Hash256 FileHash;
	};

	struct ActionFileDepositReturn {
		Hash256 FileHash;
	};

	struct ActionFilePayment {
		Hash256 FileHash;
	};

	struct DriveFile {
		Hash256 Hash;
		Hash256 ParentHash;
		uint8_t NameSize;

		const uint8_t* NamePtr() const {
			return reinterpret_cast<const uint8_t*>(this) + sizeof(DriveFile);
		}

		uint8_t* NamePtr() {
			return reinterpret_cast<uint8_t*>(this) + sizeof(DriveFile);
		}
	};

	struct ActionCreateDirectory {
		DriveFile Directory;
	};

	struct ActionRemoveDirectory {
		DriveFile Directory;
	};

	struct ActionUploadFile {
		DriveFile File;
	};

	struct ActionDownloadFile {
		DriveFile File;
	};

	struct ActionDeleteFile {
		DriveFile File;
	};

	struct ActionMoveFile {
		DriveFile Source;
		DriveFile Destination;
	};

	struct ActionCopyFile {
		DriveFile Source;
		DriveFile Destination;
	};

	uint64_t CalculateActionSize(DriveActionType type, const uint8_t* pAction);

#pragma pack(pop)
}}
