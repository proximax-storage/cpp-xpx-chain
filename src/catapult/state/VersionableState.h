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
#include "catapult/model/ImportanceHeight.h"
#include <atomic>

namespace catapult { namespace state {

	/// Version extension for state classes
	struct VersionableState {
	public:
		/// Default version 1.
		VersionableState()
				: Version(1)
		{}

		VersionableState(const uint32_t version)
				: Version(version)
		{}

		VersionableState(const VersionableState& rhs)
				: Version(rhs.Version)
		{}

		VersionableState& operator=(const VersionableState& rhs) {
			Version = rhs.Version;
			return *this;
		}

	protected:
		/// Version of this state.
		uint32_t Version;

	public:
		uint32_t GetVersion() const{ return Version; }
	};
}}
