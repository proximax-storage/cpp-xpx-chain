/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Action.h"

namespace catapult { namespace model {

	uint64_t CalculateActionSize(DriveActionType type, const uint8_t* pAction) {
		switch (type) {
			case DriveActionType::PrepareDrive : return sizeof(ActionPrepareDrive);
			case DriveActionType::DriveProlongation : return sizeof(ActionDriveProlongation);
			case DriveActionType::DriveDeposit : return 0;
			case DriveActionType::DriveDepositReturn : return 0;
			case DriveActionType::DrivePayment : return 0;
			case DriveActionType::FileDeposit : return sizeof(ActionFileDeposit);
			case DriveActionType::FileDepositReturn : return sizeof(ActionFileDepositReturn);
			case DriveActionType::FilePayment : return sizeof(ActionFilePayment);
			case DriveActionType::DriveVerification : return 0;
			case DriveActionType::CreateDirectory : return sizeof(ActionCreateDirectory) + reinterpret_cast<const ActionCreateDirectory*>(pAction)->Directory.NameSize;
			case DriveActionType::RemoveDirectory : return sizeof(ActionRemoveDirectory) + reinterpret_cast<const ActionRemoveDirectory*>(pAction)->Directory.NameSize;
			case DriveActionType::UploadFile : return sizeof(ActionUploadFile) + reinterpret_cast<const ActionUploadFile*>(pAction)->File.NameSize;
			case DriveActionType::DownloadFile : return sizeof(ActionDownloadFile) + reinterpret_cast<const ActionDownloadFile*>(pAction)->File.NameSize;
			case DriveActionType::DeleteFile : return sizeof(ActionDeleteFile) + reinterpret_cast<const ActionDeleteFile*>(pAction)->File.NameSize;
			case DriveActionType::MoveFile : {
				auto* pActionMoveFile = reinterpret_cast<const ActionMoveFile*>(pAction);
				return sizeof(ActionMoveFile) + pActionMoveFile->Source.NameSize + pActionMoveFile->Destination.NameSize;
			}
			case DriveActionType::CopyFile : {
				auto* pActionCopyFile = reinterpret_cast<const ActionCopyFile*>(pAction);
				return sizeof(ActionCopyFile) + pActionCopyFile->Source.NameSize + pActionCopyFile->Destination.NameSize;
			}
		}

		return 0;
	}
}}
