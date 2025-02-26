#pragma once

#include <stdexcept>
#include <string>

namespace vi {
	class RuntimeError : public std::runtime_error {
	public:
		RuntimeError(const std::string& msg) noexcept;
		RuntimeError(const char* msg) noexcept;
	};

	class ExternalError : public RuntimeError {
	public:
		using RuntimeError::RuntimeError;
	};

	class IOError : public RuntimeError {
	public:
		using RuntimeError::RuntimeError;
	};
}