/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/crypto/Hashes.h"
#include "catapult/functions.h"
#include "tests/TestHarness.h"
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream>

namespace catapult { namespace crypto {

#define TEST_CLASS SCM_Sha_Tests

	namespace {
		void ReadLine(std::ifstream& stream, std::string& line, uint64_t lineNumber) {
			std::getline(stream, line);
			if (!stream.good() && !stream.eof())
				CATAPULT_THROW_RUNTIME_ERROR_1("read error", lineNumber)

			if (line.size() && line[line.size() - 1] == '\r')
				line.pop_back();
		}

		template<typename THash>
		using HashFuncCustomLength = consumer<const RawBuffer&, size_t, THash&>;
		template<typename THash>
		using HashFunc = consumer<const RawBuffer&, THash&>;

		template<typename THash>
		void TestHash(const std::string& file, HashFuncCustomLength<THash> hashFunc) {
			// Arrange:
			std::ifstream inputStream("../resources/" + file + ".req");
			std::ofstream outputStream("../resources/result/" + file + ".rsp");
			uint64_t lineNumber = 0u;
			std::string line;
			uint32_t messageLength = 0u;
			std::string message;
			while (inputStream.good()) {
				++lineNumber;

				ReadLine(inputStream, line, lineNumber);
				outputStream << line << std::endl;

				if (line.empty())
					continue;

				auto pos = line.find("Len = ");
				if (std::string::npos == pos)
					continue;
				auto subStr = line.substr(pos + std::string("Len = ").size());
				messageLength = boost::lexical_cast<size_t>(subStr);

				ReadLine(inputStream, line, lineNumber);
				outputStream << line << std::endl;
				pos = line.find("Msg");
				if (std::string::npos == pos)
					CATAPULT_THROW_RUNTIME_ERROR_2("invalid line", lineNumber, line)
				if (messageLength) {
					message = line.substr(pos + std::string("Msg = ").size());
				} else {
					message.clear();
				}

				auto buffer = test::ToVector(message);
				THash hash;
				hashFunc(buffer, messageLength, hash);

				auto hashStr = test::ToString(hash);
				boost::to_lower(hashStr);
				outputStream << "MD = " << hashStr << std::endl;
			}
		}

		template<typename THash>
		void TestMonteCarlo(const std::string& file, HashFunc<THash> hashFunc) {
			// Arrange:
			std::ifstream inputStream("../resources/" + file + ".req");
			std::ofstream outputStream("../resources/result/" + file + ".rsp");
			uint64_t lineNumber = 0u;
			std::string line;
			std::string message;
			while (inputStream.good()) {
				++lineNumber;

				ReadLine(inputStream, line, lineNumber);
				outputStream << line << std::endl;

				if (line.empty())
					continue;

				auto pos = line.find("Seed = ");
				if (std::string::npos == pos)
					continue;
				outputStream << std::endl;
				message = line.substr(pos + std::string("Seed = ").size());

				for (auto i = 0u; i < 100u; ++i) {
					THash hash;
					for (auto j = 0u; j < 1000u; ++j) {
						auto buffer = test::ToVector(message);
						hashFunc(buffer, hash);
						message = test::ToString(hash);
					}

					boost::to_lower(message);
					outputStream << "j = " << i << std::endl;
					outputStream << "MD = " << message << std::endl << std::endl;
				}
			}
		}
	}

	TEST(TEST_CLASS, SHA3_512ShortMsg) {
		TestHash<Hash512>("SHA3_512ShortMsg", Sha3_512_CustomLength);
	}

	TEST(TEST_CLASS, SHA3_512LongMsg) {
		TestHash<Hash512>("SHA3_512LongMsg", Sha3_512_CustomLength);
	}

	TEST(TEST_CLASS, SHA3_512MonteCarlo) {
		TestMonteCarlo<Hash512>("SHA3_512Monte", Sha3_512);
	}

	TEST(TEST_CLASS, SHA3_256ShortMsg) {
		TestHash<Hash256>("SHA3_256ShortMsg", Sha3_256_CustomLength);
	}

	TEST(TEST_CLASS, SHA3_256LongMsg) {
		TestHash<Hash256>("SHA3_256LongMsg", Sha3_256_CustomLength);
	}

	TEST(TEST_CLASS, SHA3_256MonteCarlo) {
		TestMonteCarlo<Hash256>("SHA3_256Monte", Sha3_256);
	}

	TEST(TEST_CLASS, SHA256ShortMsg) {
		TestHash<Hash256>("SHA256ShortMsg", Sha256_CustomLength);
	}

	TEST(TEST_CLASS, SHA256LongMsg) {
		TestHash<Hash256>("SHA256LongMsg", Sha256_CustomLength);
	}

	TEST(TEST_CLASS, SHA256MonteCarlo) {
		// Arrange:
		std::string file("SHA256Monte");
		std::ifstream inputStream("../resources/" + file + ".req");
		std::ofstream outputStream("../resources/result/" + file + ".rsp");
		uint64_t lineNumber = 0u;
		std::string line;
		std::string message;
		while (inputStream.good()) {
			++lineNumber;

			ReadLine(inputStream, line, lineNumber);
			outputStream << line << std::endl;

			if (line.empty())
				continue;

			auto pos = line.find("Seed = ");
			if (std::string::npos == pos)
				continue;
			outputStream << std::endl;
			message = line.substr(pos + std::string("Seed = ").size());

			for (auto i = 0u; i < 100u; ++i) {
				Hash256 hash;
				for (auto j = 0u; j < 1000u; ++j) {
					auto buffer = test::ToVector(message);
					Sha256_CustomLength(buffer, buffer.size(), hash);
					message = test::ToString(hash);
				}

				boost::to_lower(message);
				outputStream << "j = " << i << std::endl;
				outputStream << "MD = " << message << std::endl << std::endl;
			}
		}
	}
}}