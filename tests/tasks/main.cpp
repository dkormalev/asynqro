#include "common/crash_handler.h"

#include "gtest/gtest.h"

int main(int argc, char **argv)
{
    initCrashHandler();
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
