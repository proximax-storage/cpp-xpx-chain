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
#include "ImmutableValue.h"
#include "catapult/types.h"

namespace catapult { namespace utils {

	struct XpxAmount_tag {};

	/// A non-fractional amount of xpx.
	using XpxAmount = utils::BaseValue<uint64_t, XpxAmount_tag>;

	/// Represents units of xpx.
	class XpxUnit {
	public:
		/// Raw value type.
		using ValueType = uint64_t;

	private:
		static constexpr uint64_t Microxpx_Per_Xpx = 1'000'000;

	public:
		/// Creates a zeroed xpx unit.
		constexpr XpxUnit() : m_value(0)
		{}

		/// Creates a xpx unit from \a amount microxpx.
		constexpr explicit XpxUnit(Amount amount) : m_value(amount.unwrap())
		{}

		/// Creates a xpx unit from \a amount xpx.
		constexpr explicit XpxUnit(XpxAmount amount) : m_value(amount.unwrap() * Microxpx_Per_Xpx)
		{}

		/// Creates a copy of \a rhs.
		constexpr XpxUnit(const XpxUnit& rhs) : m_value(static_cast<ValueType>(rhs.m_value))
		{}

	public:
		/// Assigns \a rhs to this.
		XpxUnit& operator=(XpxUnit rhs) {
			m_value = std::move(rhs.m_value);
			return *this;
		}

		/// Assigns \a rhs to this.
		XpxUnit& operator=(Amount rhs) {
			m_value = XpxUnit(rhs).m_value;
			return *this;
		}

		/// Assigns \a rhs to this.
		XpxUnit& operator=(XpxAmount rhs) {
			m_value = XpxUnit(rhs).m_value;
			return *this;
		}

	public:
		/// Gets the number of (whole-unit) xpx in this unit.
		constexpr XpxAmount xpx() const {
			return XpxAmount(m_value / Microxpx_Per_Xpx);
		}

		/// Gets the number of microxpx in this unit.
		constexpr Amount microxpx() const {
			return Amount(m_value);
		}

		/// Returns \c true if this unit includes fractional xpx.
		constexpr bool isFractional() const {
			return 0 != m_value % Microxpx_Per_Xpx;
		}

	public:
		/// Returns \c true if this value is equal to \a rhs.
		constexpr bool operator==(XpxUnit rhs) const {
			return m_value == rhs.m_value;
		}

		/// Returns \c true if this value is not equal to \a rhs.
		constexpr bool operator!=(XpxUnit rhs) const {
			return !(*this == rhs);
		}

		/// Insertion operator for outputting \a unit to \a out.
		friend std::ostream& operator<<(std::ostream& out, XpxUnit unit);

	private:
		utils::ImmutableValue<ValueType> m_value;
	};

	/// Tries to parse \a str into a XpxUnit (\a parsedValue).
	/// \note Only non-fractional xpx units are parsed successfully.
	bool TryParseValue(const std::string& str, XpxUnit& parsedValue);
}}
