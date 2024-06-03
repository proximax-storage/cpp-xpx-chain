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

	DEFINE_RECEIPT_TYPE(Data_Modification_Approval, Storage, Data_Modification_Approval_Download, 1);

	DEFINE_RECEIPT_TYPE(Data_Modification_Approval, Storage, Data_Modification_Approval_Refund, 2);

	DEFINE_RECEIPT_TYPE(Data_Modification_Approval, Storage, Data_Modification_Approval_Refund_Stream, 3);

	DEFINE_RECEIPT_TYPE(Data_Modification_Approval, Storage, Data_Modification_Approval_Upload, 4);

	DEFINE_RECEIPT_TYPE(Data_Modification_Cancel, Storage, Data_Modification_Cancel_Pending_Owner, 1);

	DEFINE_RECEIPT_TYPE(Data_Modification_Cancel, Storage, Data_Modification_Cancel_Pending_Replicator, 2);

	DEFINE_RECEIPT_TYPE(Data_Modification_Cancel, Storage, Data_Modification_Cancel_Queued, 3);

	DEFINE_RECEIPT_TYPE(Download_Approval, Storage, Download_Approval, 1);

	DEFINE_RECEIPT_TYPE(Download_Channel_Refund, Storage, Download_Channel_Refund, 1);

	DEFINE_RECEIPT_TYPE(Drive_Closure, Storage, Drive_Closure_Owner_Refund, 1);

	DEFINE_RECEIPT_TYPE(Drive_Closure, Storage, Drive_Closure_Replicator_Modification, 2);

	DEFINE_RECEIPT_TYPE(Drive_Closure, Storage, Drive_Closure_Replicator_Participation, 3);

	DEFINE_RECEIPT_TYPE(End_Drive_Verification, Storage, End_Drive_Verification, 1);

	DEFINE_RECEIPT_TYPE(Periodic_Payment, Storage, Periodic_Payment_Owner_Refund, 1);

	DEFINE_RECEIPT_TYPE(Periodic_Payment, Storage, Periodic_Payment_Replicator_Modification, 2);

	DEFINE_RECEIPT_TYPE(Periodic_Payment, Storage, Periodic_Payment_Replicator_Participation, 3);

	DEFINE_RECEIPT_TYPE(Replicator_Deposit, Storage, Replicator_Deposit, 1);

	DEFINE_RECEIPT_TYPE(Replicator_Deposit, Storage, Replicator_Deposit_Refund, 2);

#ifndef CUSTOM_RECEIPT_TYPE_DEFINITION
}}
#endif
