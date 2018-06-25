from conans import ConanFile, MSBuild, tools
from subprocess import call

class ShinyConan(ConanFile):
    name = "shiny"
    version = "0.1"
    license = "MIT"
    url = "https://github.com/wagk/shiny.git"
    description = "Simple Vulkan Game Engine"
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = "shared=False"
    generators = "visual_studio"

    def requirements(self):
        call("conan remote add --force bincrafters https://api.bintray.com/conan/bincrafters/public-conan")
        self.requires("conan-glfw/3.2.1@bincrafters/stable")
        self.requires("conan-glm/0.9.8.5@bincrafters/stable")
        self.requires("vulkan-sdk/1.0.46.0@alaingalvan/stable")

    def source(self):
        self.run("git clone https://github.com/wagk/shiny.git .")

    def build(self):
        msbuild = MSBuild(self)
        msbuild.build("shiny.sln")

        # Explicit way:
        # self.run('cmake %s/hello %s'
        #          % (self.source_folder, cmake.command_line))
        # self.run("cmake --build . %s" % cmake.build_config)

    def package(self):
        self.copy("*.h", dst="include", src="hello")
        self.copy("*hello.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.dylib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["hello"]

