/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
#include "catapult/model/EntityType.h"

namespace catapult { namespace model {

#endif

	/// PrepareDrive transaction.
	DEFINE_TRANSACTION_TYPE(Service, PrepareDrive, 0x1);

	/// JoinToDrive transaction.
	DEFINE_TRANSACTION_TYPE(Service, JoinToDrive, 0x2);

	/// DriveFileSystem transaction.
	DEFINE_TRANSACTION_TYPE(Service, DriveFileSystem, 0x3);

	/// FilesDeposit transaction.
	DEFINE_TRANSACTION_TYPE(Service, FilesDeposit, 0x4);

	/// EndDrive transaction.
	DEFINE_TRANSACTION_TYPE(Service, EndDrive, 0x5);

	/// DriveFilesReward transaction.
	DEFINE_TRANSACTION_TYPE(Service, DriveFilesReward, 0x6);

	/// Start verification transaction.
	DEFINE_TRANSACTION_TYPE(Service, Start_Drive_Verification, 0x7);

	/// End verification transaction.
	DEFINE_TRANSACTION_TYPE(Service, End_Drive_Verification, 0x8);

	/// Start file download transaction.
	DEFINE_TRANSACTION_TYPE(Service, StartFileDownload, 0x9);

	/// End file download transaction.
	DEFINE_TRANSACTION_TYPE(Service, EndFileDownload, 0xA);

#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
}}
#endif
