#include "Audit.hpp"
#include <stdexcept>

#ifdef _WIN32
#include <cstdio>
#include <fcntl.h>
#include <io.h>

void AnyBox::Shader::Tools::SetBinaryInput() {
    if (_setmode(_fileno(stdin), _O_BINARY) == -1) throw std::runtime_error("Cannot switch shader audit input to binary mode");
}
#else
#error Shader audit standard input requires a supported platform implementation
#endif
