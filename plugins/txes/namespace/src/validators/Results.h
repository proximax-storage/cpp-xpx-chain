/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#ifndef CUSTOM_RESULT_DEFINITION
#include "catapult/validators/ValidationResult.h"

namespace catapult { namespace validators {

#endif
/// Defines a namespace validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_NAMESPACE_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, Namespace, DESCRIPTION, CODE, None)

	// region common

	/// Validation failed because the duration has an invalid value.
	DEFINE_NAMESPACE_RESULT(Invalid_Duration, 1);

	/// Validation failed because the name is invalid.
	DEFINE_NAMESPACE_RESULT(Invalid_Name, 2);

	/// Validation failed because the name and id don't match.
	DEFINE_NAMESPACE_RESULT(Name_Id_Mismatch, 3);

	/// Validation failed because the parent is expired.
	DEFINE_NAMESPACE_RESULT(Expired, 4);

	/// Validation failed because the parent owner conflicts with the child owner.
	DEFINE_NAMESPACE_RESULT(Owner_Conflict, 5);

	/// Validation failed because the id is not the expected id generated from signer and nonce.
	DEFINE_NAMESPACE_RESULT(Id_Mismatch, 6);

	// endregion

	// region namespace

	/// Validation failed because the namespace type is invalid.
	DEFINE_NAMESPACE_RESULT(Invalid_Namespace_Type, 100);

	/// Validation failed because the root namespace has a reserved name.
	DEFINE_NAMESPACE_RESULT(Root_Name_Reserved, 101);

	/// Validation failed because the resulting namespace would exceed the maximum allowed namespace depth.
	DEFINE_NAMESPACE_RESULT(Too_Deep, 102);

	/// Validation failed because the namespace parent is unknown.
	DEFINE_NAMESPACE_RESULT(Parent_Unknown, 103);

	/// Validation failed because the namespace already exists.
	DEFINE_NAMESPACE_RESULT(Already_Exists, 150);

	/// Validation failed because the namespace is already active.
	DEFINE_NAMESPACE_RESULT(Already_Active, 151);

	/// Validation failed because an eternal namespace was received after the nemesis block.
	DEFINE_NAMESPACE_RESULT(Eternal_After_Nemesis_Block, 152);

	/// Validation failed because the maximum number of children for a root namespace was exceeded.
	DEFINE_NAMESPACE_RESULT(Max_Children_Exceeded, 153);

	/// Validation failed because alias action is invalid.
	DEFINE_NAMESPACE_RESULT(Alias_Invalid_Action, 170);

	/// Validation failed because namespace does not exist.
	DEFINE_NAMESPACE_RESULT(Alias_Namespace_Unknown, 171);

	/// Validation failed because namespace is already linked to an alias.
	DEFINE_NAMESPACE_RESULT(Alias_Already_Exists, 172);

	/// Validation failed because namespace is not linked to an alias.
	DEFINE_NAMESPACE_RESULT(Alias_Does_Not_Exist, 173);

	/// Validation failed because namespace has different owner.
	DEFINE_NAMESPACE_RESULT(Alias_Owner_Conflict, 174);

	/// Validation failed because unlink type is not consistent with existing alias.
	DEFINE_NAMESPACE_RESULT(Alias_Unlink_Type_Inconsistency, 175);

	/// Validation failed because unlink data is not consistent with existing alias.
	DEFINE_NAMESPACE_RESULT(Alias_Unlink_Data_Inconsistency, 176);

	/// Validation failed because aliased address is invalid.
	DEFINE_NAMESPACE_RESULT(Alias_Invalid_Address, 177);

	// endregion

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
