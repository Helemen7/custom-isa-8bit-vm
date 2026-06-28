#include <exception>
#include <sstream>
#include <string>

#include "../include/types.hpp"

class InvalidInstructionException : public std::exception {
      private:
	std::string message;

      public:
	InvalidInstructionException(std::size_t line, std::string_view token) {
		std::stringstream ss;
		ss << " at " << line
		   << ": error: unknown instruction mnemonic '" << token << "'";
		message = ss.str();
	}

	InvalidInstructionException(std::size_t line, Opcode opcode) {
		std::stringstream ss;
		ss << " at " << line << ": error: unknown instruction opcode '"
		   << static_cast<int>(opcode) << "'";
		message = ss.str();
	}

	const char *what() const noexcept override { return message.c_str(); }
};
