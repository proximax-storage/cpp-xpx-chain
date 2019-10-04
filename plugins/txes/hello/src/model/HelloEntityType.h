/**
*** FOR TRAINING PURPOSES ONLY
**/

#pragma once
#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
#include "catapult/model/EntityType.h"

namespace catapult { namespace model {

#endif

	/// Hello transaction.
	// expands to Entity_Type_Hello
	DEFINE_TRANSACTION_TYPE(Hello, Hello, 0x1);

#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
}}
#endif
