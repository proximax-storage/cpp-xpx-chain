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
#include <vector>
#include <map>

namespace catapult { namespace utils {

	/// Mixin to have named objects.
	class NamedObjectMixin {
	public:
		/// Creates a mixin with \a name.
		explicit NamedObjectMixin(const std::string& name) : m_name(name)
		{}

	public:
		/// Returns the name.
		const std::string& name() const {
			return m_name;
		}

	private:
		std::string m_name;
	};

	/// Extracts all names from \a namedObjects into \a names.
	template<typename TNamedObjects>
	void ExtractNames(const TNamedObjects& namedObjects, std::vector<std::string>& names) {
		names.reserve(names.size() + namedObjects.size());
		for (const auto& pObject : namedObjects)
			names.push_back(pObject->name());
	}

	/// Extracts all names from \a namedObjects.
	template<typename TNamedObjects>
	std::vector<std::string> ExtractNames(const TNamedObjects& namedObjects) {
		std::vector<std::string> names;
		ExtractNames(namedObjects, names);

		return names;
	}

	/// Extracts all names from \a namedObjects.
	template<typename TKey, typename TNamedObjects>
	std::vector<std::string> ExtractNames(const std::map<TKey, TNamedObjects>& namedObjects) {
		std::vector<std::string> names;
		for (const auto& pair : namedObjects)
			ExtractNames(pair.second, names);

		return names;
	}

	/// Reduces \a names into a single string.
	inline std::string ReduceNames(const std::vector<std::string>& names) {
		size_t numNames = 0;
		std::string result = "{";
		for (const auto& name : names) {
			if (0 != numNames++)
				result += ",";

			result += " ";
			result += name;
		}

		if (0 != numNames)
			result += " ";

		result += "}";
		return result;
	}
}}
