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
#include <string>
#include <cstdint>

namespace catapult { namespace test {

	/// Prepares the storage by copying seed data into the \a destination directory.
	void PrepareStorage(const std::string& destination, std::string sourceDir = "");

	/// Fakes file-based chain located at \a destination to \a height
	/// by setting proper value in index.dat and filling 00000/hashes.dat.
	void FakeHeight(const std::string& destination, uint64_t height);
}}
