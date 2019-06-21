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

#include "BufferedFileStream.h"
#include "catapult/utils/Logging.h"
#include "catapult/exceptions.h"

namespace catapult { namespace io {

	BufferedOutputFileStream::BufferedOutputFileStream(RawFile&& rawFile, size_t bufferSize)
			: m_rawFile(std::move(rawFile))
			, m_buffer(bufferSize)
			, m_bufferPosition(0)
	{}

	void BufferedOutputFileStream::flush() {
		m_rawFile.write({ m_buffer.data(), m_bufferPosition });
		m_bufferPosition = 0;
	}

	void BufferedOutputFileStream::write(const RawBuffer& buffer) {
		// bypass caching if write buffer is larger than internal buffer
		if (buffer.Size > m_buffer.size()) {
			flush();
			m_rawFile.write(buffer);
			return;
		}

		auto bytesLeft = m_buffer.size() - m_bufferPosition;
		auto bytesToCopy = std::min(buffer.Size, bytesLeft);

		// copy chunk to internal buffer
		std::memcpy(m_buffer.data() + m_bufferPosition, buffer.pData, bytesToCopy);
		m_bufferPosition += bytesToCopy;

		// write out if full
		if (m_buffer.size() == m_bufferPosition) {
			flush();

			if (bytesToCopy < buffer.Size) {
				// copy rest
				auto remainingRequestBytes = buffer.Size - bytesToCopy;
				std::memcpy(m_buffer.data(), buffer.pData + bytesToCopy, remainingRequestBytes);
				m_bufferPosition = remainingRequestBytes;
			}
		}
	}

	BufferedInputFileStream::BufferedInputFileStream(RawFile&& rawFile, size_t bufferSize)
			: m_rawFile(std::move(rawFile))
			, m_buffer(bufferSize)
			, m_bufferPosition(0)
			, m_numBytesInBuffer(0)
	{}

	bool BufferedInputFileStream::eof() const {
		return m_rawFile.position() == m_rawFile.size();
	}

	namespace {
		[[noreturn]]
		void ThrowReadError(size_t requestedBytes, size_t availableBytes) {
			CATAPULT_THROW_AND_LOG_2(
					catapult_file_io_error,
					"couldn't read from file, requested size vs available",
					requestedBytes,
					availableBytes);
		}
	}

	void BufferedInputFileStream::read(const MutableRawBuffer& buffer) {
		size_t outputPosition = 0; // bytes from m_buffer written so far to buffer

		auto numAvailableBytes = m_numBytesInBuffer - m_bufferPosition;
		auto bytesToCopy = std::min(buffer.Size, numAvailableBytes);

		// most common case: if there's data left in m_buffer, copy from there
		if (numAvailableBytes) {
			std::memcpy(buffer.pData, m_buffer.data() + m_bufferPosition, bytesToCopy);
			outputPosition += bytesToCopy;
			m_bufferPosition += bytesToCopy;

			// if all requested data was read from m_buffer, nothing else to do
			if (buffer.Size == bytesToCopy)
				return;
		}

		// bypass caching if remaining read request is larger than internal buffer
		auto remainingRequestedBytes = buffer.Size - outputPosition;
		if (remainingRequestedBytes > m_buffer.size()) {
			m_rawFile.read({ buffer.pData + outputPosition, remainingRequestedBytes });
			return;
		}

		// refill the internal buffer and complete the read request
		m_numBytesInBuffer = std::min<size_t>(m_buffer.size(), m_rawFile.size() - m_rawFile.position());
		m_rawFile.read({ m_buffer.data(), m_numBytesInBuffer });
		m_bufferPosition = 0;

		if (remainingRequestedBytes > m_numBytesInBuffer)
			ThrowReadError(buffer.Size, m_numBytesInBuffer + outputPosition);

		std::memcpy(buffer.pData + outputPosition, m_buffer.data(), remainingRequestedBytes);
		m_bufferPosition += remainingRequestedBytes;
	}
}}
