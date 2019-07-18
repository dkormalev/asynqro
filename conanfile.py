from conans import ConanFile, CMake, tools
from conans.errors import ConanInvalidConfiguration
import re

def get_version():
    try:
        content = tools.load("CMakeLists.txt")
        version = re.search(r"project\(.* VERSION ([^ )]*)", content).group(1)
        return version.strip()
    except:
        return None

class AsynqroConan(ConanFile):
    name = "asynqro"
    version = get_version()
    license = "BSD-3-Clause"
    author = "Denis Kormalev (kormalev.denis@gmail.com)"
    url = "https://github.com/dkormalev/asynqro.git"
    description = "Asynqro is a small library with purpose to make C++ programming easier by giving developers rich monadic Future API"
    topics = ("future", "promise", "monad", "functional", "thread-pool", "task-scheduling")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "build_tests": [True, False]}
    default_options = {"shared": False, "build_tests": False}
    generators = "cmake"
    exports = ["LICENSE"]
    exports_sources = ["cmake*", "include*", "src*", "CMakeLists.txt", "tests*"]

    def configure(self):
        compiler_version = tools.Version(str(self.settings.compiler.version))
        if self.settings.compiler == "Visual Studio":
            if compiler_version < "15":
                raise ConanInvalidConfiguration("MSVC < 15 not supported")
        if self.settings.compiler == "gcc":
            if compiler_version < "7":
                raise ConanInvalidConfiguration("gcc < 7 not supported")
        if self.settings.compiler == "clang":
            if compiler_version < "6":
                raise ConanInvalidConfiguration("clang < 6 not supported")
        if self.settings.compiler == "apple-clang":
            if compiler_version < "10.0":
                raise ConanInvalidConfiguration("apple-clang < 10.0 not supported")

    def _configure_cmake(self):
        cmake = CMake(self)
        if self.options.build_tests:
            cmake.definitions["ASYNQRO_BUILD_TESTS"] = True
        cmake.configure()
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        self.copy("LICENSE", dst="licenses")
        cmake = self._configure_cmake()
        cmake.install()

    def package_id(self):
        del self.info.options.build_tests

    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
        if self.settings.os == "Linux":
            self.cpp_info.cxxflags.append("-pthread")
            self.cpp_info.sharedlinkflags.append("-pthread")
