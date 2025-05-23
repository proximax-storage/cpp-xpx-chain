/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/model/EntityType.h"
#include <boost/filesystem.hpp>

namespace catapult { namespace config {

	using SupportedEntityVersions = std::map<model::EntityType, VersionSet>;

	/// Loads supported entity versions from \a path.
	SupportedEntityVersions LoadSupportedEntityVersions(const boost::filesystem::path& path);

	/// Loads supported entity versions from \a path.
	SupportedEntityVersions LoadSupportedEntityVersions(std::istream& inputStream);
}}
