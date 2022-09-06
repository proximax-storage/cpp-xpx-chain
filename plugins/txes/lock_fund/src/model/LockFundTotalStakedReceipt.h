/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "catapult/model/Receipt.h"
namespace catapult { namespace model {
		/// Binary layout for an amount receipt to record the total locked harvesting mosaic
		struct TotalStakedReceipt : public Receipt {
		public:
			/// Creates a receipt around \a amount and \a lockedAmount.
			TotalStakedReceipt(
					ReceiptType receiptType,
					catapult::Amount amount)
					: Amount(amount){
				Size = sizeof(TotalStakedReceipt);
				Version = 1;
				Type = receiptType;
			}

		public:
			/// Amount of mosaic.
			catapult::Amount Amount;
		};
}}

