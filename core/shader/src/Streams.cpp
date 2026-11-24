#include <cstdio>

extern "C" std::FILE* HostStandardOutputFile_nid_no_patch() {
    return stdout;
}

extern "C" std::FILE* HostStandardErrorFile_nid_no_patch() {
    return stderr;
}
