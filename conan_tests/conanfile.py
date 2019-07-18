import os

from conans import ConanFile, CMake, tools

class AsynqroTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"

    def imports(self):
        self.copy("*.dll", dst="bin", src="bin")
        self.copy("*.dll", dst="bin", src="lib")
        self.copy("*.so*", dst="bin", src="lib")
        self.copy("*.dylib*", dst="bin", src="lib")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if (not tools.cross_building(self.settings)) or self.settings.os == "Windows":
            bin_path = os.path.join("bin", "asynqro_tester")
            if self.settings.os == "Windows":
                self.run("%s.exe" % bin_path, run_environment=True)
            else:
                self.run(bin_path, run_environment=True)
