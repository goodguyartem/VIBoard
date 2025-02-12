#include "Exceptions.h"

#include <SDL3/SDL.h>

namespace vi {
	RuntimeError::RuntimeError(const std::string& msg) noexcept
		: RuntimeError(msg.c_str()) {
	}

	RuntimeError::RuntimeError(const char* msg) noexcept
		: runtime_error(msg) {
#ifdef _DEBUG
		SDL_TriggerBreakpoint();
#endif
	}
}