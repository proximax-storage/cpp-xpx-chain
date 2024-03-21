/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#ifndef CUSTOM_RECEIPT_TYPE_DEFINITION
#include "catapult/model/ReceiptType.h"

namespace catapult { namespace model {

#endif

	DEFINE_RECEIPT_TYPE(Storage, Storage, Data_Modification_Approval_Download, 1);

	DEFINE_RECEIPT_TYPE(Storage, Storage, Data_Modification_Approval_Refund, 2);

	DEFINE_RECEIPT_TYPE(Storage, Storage, Data_Modification_Approval_Refund_Stream, 3);

	DEFINE_RECEIPT_TYPE(Storage, Storage, Data_Modification_Approval_Upload, 4);

	DEFINE_RECEIPT_TYPE(Storage, Storage, Data_Modification_Cancel_Pending_Owner, 5);

	DEFINE_RECEIPT_TYPE(Storage, Storage, Data_Modification_Cancel_Pending_Replicator, 6);

	DEFINE_RECEIPT_TYPE(Storage, Storage, Data_Modification_Cancel_Queued, 7);

	DEFINE_RECEIPT_TYPE(Storage, Storage, Download_Approval, 8);

	DEFINE_RECEIPT_TYPE(Storage, Storage, Download_Channel_Refund, 9);

	DEFINE_RECEIPT_TYPE(Storage, Storage, Drive_Closure_Owner_Refund, 10);

	DEFINE_RECEIPT_TYPE(Storage, Storage, Drive_Closure_Replicator_Modification, 11);

	DEFINE_RECEIPT_TYPE(Storage, Storage, Drive_Closure_Replicator_Participation, 12);

	DEFINE_RECEIPT_TYPE(Storage, Storage, End_Drive_Verification, 13);

	DEFINE_RECEIPT_TYPE(Storage, Storage, Periodic_Payment_Owner_Refund, 14);

	DEFINE_RECEIPT_TYPE(Storage, Storage, Periodic_Payment_Replicator_Modification, 15);

	DEFINE_RECEIPT_TYPE(Storage, Storage, Periodic_Payment_Replicator_Participation, 16);

#ifndef CUSTOM_RECEIPT_TYPE_DEFINITION
}}
#endif
