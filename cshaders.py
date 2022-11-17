import os
import platform

flag = False

bin_dir = "cmake-build-debug"
src_dir = "Resources/Shaders"

target_vk = "vulkan1.3"
raytracing = ["raytrace.rchit", "raytrace.rgen", "raytrace.rmiss"]
shaders = ["phong.vert", "phong.frag", "basic.vert", "basic.frag", "tex.frag"]


def compile_rt():
    print(f"Compiling ({len(raytracing)}) ray tracing shader(s)...")
    for r in raytracing:
        command = f"glslangValidator -g -o {bin_dir}/{r}.spv -V {src_dir}/{r} --target-env {target_vk}"
        if flag:
            print(f"\t{command}")
        os.popen(command)


def compile_sh():
    print(f"Compiling ({len(shaders)}) shader(s)...")
    for s in shaders:
        command = f"glslc -o {bin_dir}/{s}.spv {src_dir}/{s}"
        if flag:
            print(f"\t{command}")
        os.popen(command)


def main():
    compile_sh()
    if platform.system() != "Darwin":
        compile_rt()


main()
