/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#ifndef CUSTOM_RESULT_DEFINITION
#include "catapult/validators/ValidationResult.h"

namespace catapult { namespace validators {

#endif

/// Defines a metadata validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_METADATA_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, Metadata, DESCRIPTION, CODE, None)

	/// Validation failed because the metadata type is invalid.
	DEFINE_METADATA_RESULT(Invalid_Metadata_Type, 1);

	/// Validation failed because a modification type is invalid.
	DEFINE_METADATA_RESULT(Modification_Type_Invalid, 2);

	/// Validation failed because a modification type is invalid.
	DEFINE_METADATA_RESULT(Modification_Key_Invalid, 3);

	/// Validation failed because a modification type is invalid.
	DEFINE_METADATA_RESULT(Modification_Value_Invalid, 4);

	/// Validation failed because a modifications contains modification with the same key.
	DEFINE_METADATA_RESULT(Modification_Key_Redundant, 10);

	/// Validation failed because a modifications contains modification with the same key and value.
	DEFINE_METADATA_RESULT(Modification_Value_Redundant, 11);

	/// Validation failed because a modification remove not existing key.
	DEFINE_METADATA_RESULT(Remove_Not_Existing_Key, 12);

	/// Validation failed because a modification of address is not permitted.
	DEFINE_METADATA_RESULT(Address_Modification_Not_Permitted, 16);

	/// Validation failed because a modification of mosaic is not permitted.
	DEFINE_METADATA_RESULT(Mosaic_Modification_Not_Permitted, 17);

	/// Validation failed because a modification of namespace is not permitted.
	DEFINE_METADATA_RESULT(Namespace_Modification_Not_Permitted, 18);

	/// Validation failed because address is not found.
	DEFINE_METADATA_RESULT(Address_Not_Found, 21);

	/// Validation failed because mosaic is not found.
	DEFINE_METADATA_RESULT(Mosaic_Not_Found, 22);

	/// Validation failed because namespace is not found.
	DEFINE_METADATA_RESULT(Namespace_Not_Found, 23);

	/// Validation failed because metadata contains to much keys.
	DEFINE_METADATA_RESULT(Too_Much_Keys, 30);

	/// Validation failed because plugin configuration data is malformed.
	DEFINE_METADATA_RESULT(Plugin_Config_Malformed, 31);

	/// Validation failed because mosaic id is malformed.
	DEFINE_METADATA_RESULT(MosaicId_Malformed, 32);

	/// Validation failed because namespace id is malformed.
	DEFINE_METADATA_RESULT(NamespaceId_Malformed, 33);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
