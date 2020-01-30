/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/EmbeddedTransaction.h"
#include "catapult/model/ExtendedEmbeddedTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include <string>

namespace catapult { namespace plugins {

	inline const model::ExtendedEmbeddedTransaction& ConvertEmbeddedTransaction(
			const model::EmbeddedTransaction& subTransaction, const Timestamp& deadline, model::NotificationSubscriber& sub) {

		auto pUnique = sub.mempool().malloc<uint8_t>(subTransaction.Size + sizeof(deadline));
		std::memcpy(
				pUnique,
				reinterpret_cast<const uint8_t*>(&subTransaction),
				sizeof(subTransaction)
		);
		std::memcpy(
				pUnique + sizeof(subTransaction),
				reinterpret_cast<const uint8_t*>(&deadline),
				sizeof(deadline)
		);
		std::memcpy(
				pUnique + sizeof(model::ExtendedEmbeddedTransaction),
				reinterpret_cast<const uint8_t*>(&subTransaction) + sizeof(subTransaction),
				subTransaction.Size - sizeof(subTransaction)
		);
		model::ExtendedEmbeddedTransaction& tx = *reinterpret_cast<model::ExtendedEmbeddedTransaction*>(pUnique);
		tx.Size = subTransaction.Size + sizeof(deadline);

		return tx;
	}
}}
