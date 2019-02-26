#include "gtest/gtest.h"

#ifdef ASYNQRO_QT_SUPPORT
#    include <QDebug>
#endif

#if (defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)) || defined(Q_OS_MAC)
#    include <ctime>
#    include <cxxabi.h>
#    include <execinfo.h>
#    include <fcntl.h>
#    include <signal.h>
#    include <stdio.h>
#    include <sys/ucontext.h>
#    include <unistd.h>
#endif

#if (defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)) || defined(Q_OS_MAC)
constexpr int BACKTRACE_MAX_SIZE = 50;

static void signalHandler(int sig, siginfo_t *info, void *context)
{
    static bool handlerAlreadyCalled = false;
    if (handlerAlreadyCalled)
        return;
    handlerAlreadyCalled = true;

    alarm(10);
    char *homeDir = getenv("HOME");
    ucontext_t *uc = (ucontext_t *)context;
#    ifdef Q_OS_LINUX
    void *caller = (void *)uc->uc_mcontext.fpregs->rip;
#    else
    void *caller = (void *)uc->uc_mcontext->__ss.__rip;
#    endif

    QString toLog = QStringLiteral("#######################################");
    write(STDOUT_FILENO, toLog.toLatin1().constData(), toLog.length());
    write(STDOUT_FILENO, "\n", 1);
    toLog = QStringLiteral("signal %1 (%2), address is 0x%3 from 0x%4")
                .arg(sig)
                .arg(strsignal(sig))
                .arg((unsigned long long)info->si_addr, 0, 16)
                .arg((unsigned long long)caller, 0, 16);
    write(STDOUT_FILENO, toLog.toLatin1().constData(), toLog.length());
    write(STDOUT_FILENO, "\n", 1);

    void *backtraceInfo[BACKTRACE_MAX_SIZE];
    int size = backtrace(backtraceInfo, BACKTRACE_MAX_SIZE);

    backtraceInfo[0] = backtraceInfo[1];
    backtraceInfo[1] = caller;

    char **backtraceArray = backtrace_symbols(backtraceInfo, size);

    if (!backtraceArray) {
        _Exit(EXIT_FAILURE);
    }

    for (int i = 0; i < size; ++i) {
#    ifdef Q_OS_LINUX
        QRegExp re("^(.+)\\((.*)\\+([x0-9a-fA-F]*)\\)\\s+\\[(.+)\\]\\s*$");
#    else
        QRegExp re("^\\d*\\s+(.+)\\s+(.+)\\s+(.+)\\s+\\+\\s+(\\d*)\\s*$");
#    endif

        if (re.indexIn(backtraceArray[i]) < 0) {
            toLog = QStringLiteral("[trace] #%1) %2").arg(i).arg(backtraceArray[i]);
            write(STDOUT_FILENO, toLog.toLatin1().constData(), toLog.length());
            write(STDOUT_FILENO, "\n", 1);
        } else {
#    ifdef Q_OS_LINUX
            QString mangledName = re.cap(2).trimmed();
#    else
            QString mangledName = re.cap(3).trimmed();
#    endif
            char *name = abi::__cxa_demangle(mangledName.toLatin1().constData(), 0, 0, 0);
#    ifdef Q_OS_LINUX
            toLog = QString("[trace] #%1) %2 : %3+%4 (%5)")
                        .arg(i)
                        .arg(re.cap(1).trimmed()) //scope
                        .arg(name ? name : mangledName) //name
                        .arg(re.cap(3).trimmed()) // clazy:exclude=qstring-arg
                        .arg(re.cap(4).trimmed()); //offset and address
#    else
            toLog = QStringLiteral("[trace] #%1) %2 : %3+%4 (%5)")
                        .arg(i)
                        .arg(re.cap(1).trimmed()) //scope
                        .arg(name ? name : mangledName) //name
                        .arg(re.cap(4).trimmed()) // clazy:exclude=qstring-arg
                        .arg(re.cap(2).trimmed()); //offset and address
#    endif
            write(STDOUT_FILENO, toLog.toLatin1().constData(), toLog.length());
            write(STDOUT_FILENO, "\n", 1);
            free(name);
        }
    }

    free(backtraceArray);
    _Exit(EXIT_FAILURE);
}
#endif

int main(int argc, char **argv)
{
#if (defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)) || defined(Q_OS_MAC)
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
        qDebug() << "No segfault handler is on your back.";
    if (sigaction(SIGABRT, &sigAbrtAction, (struct sigaction *)NULL) != 0)
        qDebug() << "No abort handler is on your back.";
    if (sigaction(SIGFPE, &sigFpeAction, (struct sigaction *)NULL) != 0)
        qDebug() << "No fp error handler is on your back.";
    if (sigaction(SIGILL, &sigIllAction, (struct sigaction *)NULL) != 0)
        qDebug() << "No illegal instruction handler is on your back.";
#endif
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
