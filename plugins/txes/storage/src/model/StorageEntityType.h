/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
#include "catapult/model/EntityType.h"

namespace catapult { namespace model {

#endif

	/// PrepareBcDrive transaction.
	DEFINE_TRANSACTION_TYPE(Storage, PrepareBcDrive, 0x1);

	/// DataModification transaction.
	DEFINE_TRANSACTION_TYPE(Storage, DataModification, 0x2);

	/// Download transaction.
	DEFINE_TRANSACTION_TYPE(Storage, Download, 0x3);

	/// DataModificationApproval transaction.
	DEFINE_TRANSACTION_TYPE(Storage, DataModificationApproval, 0x4);
	
	/// DataModificationCancel transaction.
	DEFINE_TRANSACTION_TYPE(Storage, DataModificationCancel, 0x5);

	/// ReplicatorOnboarding transaction.
	DEFINE_TRANSACTION_TYPE(Storage, ReplicatorOnboarding, 0x6);

	/// FinishDownload transaction.
	DEFINE_TRANSACTION_TYPE(Storage, FinishDownload, 0x7);

	/// DownloadPayment transaction.
	DEFINE_TRANSACTION_TYPE(Storage, DownloadPayment, 0x8);

	/// StoragePayment transaction.
	DEFINE_TRANSACTION_TYPE(Storage, StoragePayment, 0x9);

	/// DataModificationSingleApproval transaction.
	DEFINE_TRANSACTION_TYPE(Storage, DataModificationSingleApproval, 0xA);

	/// VerificationPayment transaction.
	DEFINE_TRANSACTION_TYPE(Storage, VerificationPayment, 0xB);

	/// DownloadApproval transaction.
	DEFINE_TRANSACTION_TYPE(Storage, DownloadApproval, 0xC);

#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
}}
#endif
