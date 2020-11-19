/* Copyright 2020, Denis Kormalev
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of
 * conditions and the following disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *     * Neither the name of the copyright holders nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#if defined(__APPLE__) || defined(__linux__)
#    include <cxxabi.h>
#    include <execinfo.h>
#    include <iostream>
#    include <regex>
#    include <signal.h>
#    include <sys/ucontext.h>
#    include <unistd.h>
#endif

#if defined(__APPLE__) || defined(__linux__)
void signalHandler(int sig, siginfo_t *info, void *context)
{
    constexpr static int BACKTRACE_MAX_SIZE = 50;
    static bool handlerAlreadyCalled = false;
    if (handlerAlreadyCalled)
        return;
    handlerAlreadyCalled = true;

    alarm(10);

    std::cerr << "#######################################" << std::endl;
    std::cerr << "signal " << sig << " (" << strsignal(sig) << ")" << std::endl;

    void *backtraceInfo[BACKTRACE_MAX_SIZE];
    int size = backtrace(backtraceInfo, BACKTRACE_MAX_SIZE);

#    ifdef __APPLE__
    backtraceInfo[1] = (void *)((ucontext_t *)context)->uc_mcontext->__ss.__rip;
#    endif

    char **backtraceArray = backtrace_symbols(backtraceInfo, size);

    if (!backtraceArray) {
        _Exit(EXIT_FAILURE);
    }

#    ifdef __linux__
    const int scope = 1;
    const int address = 4;
    const int offset = 3;
#    else
    const int scope = 1;
    const int address = 2;
    const int offset = 4;
#    endif

    for (int i = 1; i < size; ++i) {
#    ifdef __linux__
        std::regex re(R"(^(.+)\((.*)\+([x0-9a-fA-F]*)\)\s+\[(.+)\]\s*$)");
#    else
        std::regex re(R"(^\d*\s+(.*[^\s])\s+(.+)\s+(.+)\s+\+\s+(\d*)\s*$)");
#    endif

        std::cmatch traceMatch;
        if (!std::regex_match(backtraceArray[i], traceMatch, re)) {
            std::cerr << "[trace] #" << i << ") " << backtraceArray[i] << ")" << std::endl;
        } else {
#    ifdef __linux__
            std::string mangledName = traceMatch[2].str();
#    else
            std::string mangledName = traceMatch[3].str();
#    endif
            char *name = abi::__cxa_demangle(mangledName.data(), 0, 0, 0);
            std::cerr << "[trace] #" << i << ") " << traceMatch[scope].str() << " : " << (name ? name : mangledName)
                      << "+" << traceMatch[offset].str() << " (" << traceMatch[address].str() << ")" << std::endl;
        }
    }
    _Exit(EXIT_FAILURE);
}
#endif

inline void initCrashHandler()
{
#if defined(__APPLE__) || defined(__linux__)
    static struct sigaction sigSegvAction;
    sigSegvAction.sa_sigaction = signalHandler;
    sigSegvAction.sa_flags = SA_SIGINFO;
    static struct sigaction sigAbrtAction;
    sigAbrtAction.sa_sigaction = signalHandler;
    sigAbrtAction.sa_flags = SA_SIGINFO;
    static struct sigaction sigFpeAction;
    sigFpeAction.sa_sigaction = signalHandler;
    sigFpeAction.sa_flags = SA_SIGINFO;
    static struct sigaction sigIllAction;
    sigIllAction.sa_sigaction = signalHandler;
    sigIllAction.sa_flags = SA_SIGINFO;

    if (sigaction(SIGSEGV, &sigSegvAction, (struct sigaction *)NULL) != 0)
        std::cerr << "No segfault handler is on your back." << std::endl;
    if (sigaction(SIGABRT, &sigAbrtAction, (struct sigaction *)NULL) != 0)
        std::cerr << "No abort handler is on your back." << std::endl;
    if (sigaction(SIGFPE, &sigFpeAction, (struct sigaction *)NULL) != 0)
        std::cerr << "No fp error handler is on your back." << std::endl;
    if (sigaction(SIGILL, &sigIllAction, (struct sigaction *)NULL) != 0)
        std::cerr << "No illegal instruction handler is on your back." << std::endl;
#endif
}
