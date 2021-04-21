/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#ifndef CUSTOM_RECEIPT_TYPE_DEFINITION
#include "catapult/model/ReceiptType.h"

namespace catapult { namespace model {

#endif

	DEFINE_RECEIPT_TYPE(Drive, Storage, Drive_State, 1);

	DEFINE_RECEIPT_TYPE(Drive, Storage, Drive_Deposit_Credit, 2);

	DEFINE_RECEIPT_TYPE(Drive, Storage, Drive_Deposit_Debit, 3);

	DEFINE_RECEIPT_TYPE(Drive, Storage, Drive_Reward_Transfer_Credit, 4);

	DEFINE_RECEIPT_TYPE(Drive, Storage, Drive_Reward_Transfer_Debit, 5);

	DEFINE_RECEIPT_TYPE(Drive, Storage, Drive_Download_Started, 6);

	DEFINE_RECEIPT_TYPE(Drive, Storage, Drive_Download_Completed, 7);

	DEFINE_RECEIPT_TYPE(Drive, Storage, Drive_Download_Expired, 8);

#ifndef CUSTOM_RECEIPT_TYPE_DEFINITION
}}
#endif
