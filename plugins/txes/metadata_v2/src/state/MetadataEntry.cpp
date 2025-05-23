/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
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

#include "MetadataEntry.h"

namespace catapult { namespace state {

	MetadataEntry::MetadataEntry(const MetadataKey& key, VersionType version)
		: m_key(key)
		, m_version(version)
	{}

	VersionType MetadataEntry::version() const {
		return m_version;
	}

	void MetadataEntry::setVersion(VersionType version) {
		m_version = version;
	}

	const MetadataKey& MetadataEntry::key() const {
		return m_key;
	}

	const MetadataValue& MetadataEntry::value() const {
		return m_value;
	}

	MetadataValue& MetadataEntry::value() {
		return m_value;
	}

	bool MetadataEntry::isImmutable() const
	{
		return m_immutable;
	}

	void MetadataEntry::setImmutable(bool isImmutable)
	{
		m_immutable = isImmutable;
	}
}}
