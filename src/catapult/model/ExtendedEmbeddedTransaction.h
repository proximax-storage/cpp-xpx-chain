/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "EmbeddedTransaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for an extended embedded transaction (only blockchain usage, to calculate hash of inner transaction).
	struct ExtendedEmbeddedTransaction : public EmbeddedTransaction {

        /// Transaction deadline.
        Timestamp Deadline;
	};

#pragma pack(pop)
}}
