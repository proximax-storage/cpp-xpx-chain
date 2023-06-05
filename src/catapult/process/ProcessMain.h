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
#include <boost/filesystem/path.hpp>
#include <functional>
#include <memory>

namespace catapult {
	namespace config { class BlockchainConfigurationHolder; }
	namespace crypto { class KeyPair; }
}

namespace catapult { namespace process {

	/// Process options.
	enum class ProcessOptions {
		/// Exit immediately after process host creation.
		Exit_After_Process_Host_Creation,

		/// Wait for termination signal before exiting.
		Exit_After_Termination_Signal
	};

	/// Prototype for creating a process host.
	/// \note Return value is a shared_ptr because unique_ptr of void is not allowed.
	using CreateProcessHost = std::function<std::shared_ptr<void> (const std::shared_ptr<config::BlockchainConfigurationHolder>&, const crypto::KeyPair&)>;

	/// Prototype for preparing and validating a host configuration.
	/// \note Return value is a shared_ptr because unique_ptr of void is not allowed.
	using CreateProcessHostConfig = std::function<std::shared_ptr<config::BlockchainConfigurationHolder> (const boost::filesystem::path&, const std::string&)>;

	/// Main entry point for a catapult process named \a host with default process options.
	/// \a argc commmand line arguments are accessible via \a argv.
	/// \a createProcessHost creates the process host.
	int ProcessMain(int argc, const char** argv, const std::string& host, const CreateProcessHostConfig& createProcessConfig, const CreateProcessHost& createProcessHost);

	/// Main entry point for a catapult process named \a host with specified process options (\a processOptions).
	/// \a argc commmand line arguments are accessible via \a argv.
	/// \a createProcessHost creates the process host.
	int ProcessMain(
			int argc,
			const char** argv,
			const std::string& host,
			ProcessOptions processOptions,
			const CreateProcessHostConfig& createProcessConfig,
			const CreateProcessHost& createProcessHost);
}}
