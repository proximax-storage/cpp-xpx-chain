/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "MetadataV1SharedTransaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for an address Metadata transaction body.
	DEFINE_METADATA_TRANSACTION(Address, Address, UnresolvedAddress)

#pragma pack(pop)
}}
