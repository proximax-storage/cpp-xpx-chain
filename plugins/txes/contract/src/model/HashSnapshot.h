/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a hash snapshot.
	struct HashSnapshot {
		///  The hash of file when snapshot was done.
		Hash256 Hash;

		/// The height when snapshot was done.
		Height HashHeight;

	public:
		bool operator==(const HashSnapshot& rhs) {
			return Hash == rhs.Hash && HashHeight.unwrap() == rhs.HashHeight.unwrap();
		}

		friend bool operator ==(const HashSnapshot& lhs,  const HashSnapshot& rhs) {
			return lhs.Hash == rhs.Hash && lhs.HashHeight.unwrap() == rhs.HashHeight.unwrap();
		}
	};

#pragma pack(pop)
}}
