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
#include "Casting.h"
#include <iosfwd>
#include <stdint.h>

namespace catapult { namespace utils {

	/// Represents a file size.
	class FileSize final {
	private:
		constexpr explicit FileSize(uint64_t bytes) : m_bytes(bytes)
		{}

	public:
		/// Creates a default (zero) file size.
		constexpr FileSize() : FileSize(0)
		{}

	public:
		/// Creates a file size from the given number of \a terabytes.
		static constexpr FileSize FromTerabytes(uint64_t terabytes) {
			return FromGigabytes(terabytes * 1024);
		}

		/// Creates a file size from the given number of \a gigabytes.
		static constexpr FileSize FromGigabytes(uint64_t gigabytes) {
			return FromMegabytes(gigabytes * 1024);
		}

		/// Creates a file size from the given number of \a megabytes.
		static constexpr FileSize FromMegabytes(uint64_t megabytes) {
			return FromKilobytes(megabytes * 1024);
		}

		/// Creates a file size from the given number of \a kilobytes.
		static constexpr FileSize FromKilobytes(uint64_t kilobytes) {
			return FromBytes(kilobytes * 1024);
		}

		/// Creates a file size from the given number of \a bytes.
		static constexpr FileSize FromBytes(uint64_t bytes) {
			return FileSize(bytes);
		}

	public:
	    /// Returns the number of terabytes rounded up.
	    constexpr uint64_t terabytesCeil() const {
		    return gigabytes() / 1024;
		}

		/// Returns the number of terabytes.
		constexpr uint64_t terabytes() const {
			return gigabytes() / 1024;
		}

		/// Returns the number of gigabytes rounded up.
		constexpr uint64_t gigabytesCeil() const {
		    return (megabytesCeil() + 1023) / 1024;
		}

		/// Returns the number of gigabytes.
		constexpr uint64_t gigabytes() const {
			return megabytes() / 1024;
		}

		/// Returns the number of megabytes rounded up.
		constexpr uint64_t megabytesCeil() const {
		    return (kilobytesCeil() + 1023) / 1024;
		}

		/// Returns the number of megabytes .
		constexpr uint64_t megabytes() const {
			return kilobytes() / 1024;
		}

		/// Returns the number of kilobytes rounded up.
		constexpr uint64_t kilobytesCeil() const {
		    return (bytes() + 1023) / 1024;
		}

		/// Returns the number of kilobytes.
		constexpr uint64_t kilobytes() const {
			return bytes() / 1024;
		}

		/// Returns the number of bytes.
		constexpr uint64_t bytes() const {
			return m_bytes;
		}

		/// Returns the number of bytes as a uint32_t.
		uint32_t bytes32() const {
			return checked_cast<uint64_t, uint32_t>(m_bytes);
		}

	public:
		/// Returns \c true if this file size is equal to \a rhs.
		constexpr bool operator==(const FileSize& rhs) const {
			return m_bytes == rhs.m_bytes;
		}

		/// Returns \c true if this file size is not equal to \a rhs.
		constexpr bool operator!=(const FileSize& rhs) const {
			return !(*this == rhs);
		}

		/// Returns \c true if this file size is greater than or equal to \a rhs.
		constexpr bool operator>=(const FileSize& rhs) const {
			return m_bytes >= rhs.m_bytes;
		}

		/// Returns \c true if this file size is greater than \a rhs.
		constexpr bool operator>(const FileSize& rhs) const {
			return m_bytes > rhs.m_bytes;
		}

		/// Returns \c true if this file size is less than or equal to \a rhs.
		constexpr bool operator<=(const FileSize& rhs) const {
			return m_bytes <= rhs.m_bytes;
		}

		/// Returns \c true if this file size is less than \a rhs.
		constexpr bool operator<(const FileSize& rhs) const {
			return m_bytes < rhs.m_bytes;
		}

	private:
		uint64_t m_bytes;
	};

	/// Insertion operator for outputting \a fileSize to \a out.
	std::ostream& operator<<(std::ostream& out, const FileSize& fileSize);
}}
