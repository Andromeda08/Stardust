import os
import glob
import platform

# shader source file extensions
rt_extensions = ["rchit", "rgen", "rmiss"]
sh_extensions = ["vert", "frag", "glsl", "geom", "comp"]

# configuration
debug_print = False
bin_dir = "cmake-build-debug"
src_dir = "Resources/Shaders"
target_vk = "vulkan1.3"

# found & sorted shaders
_raytracing = []
_shaders = []


def sort_shaders():
    all_shaders = glob.glob(f"{src_dir}/*")
    for shader in all_shaders:
        extension = shader.rsplit('.')[-1]
        file = shader.rsplit('\\')[-1]
        if extension in rt_extensions:
            _raytracing.append(file)
        elif extension in sh_extensions:
            _shaders.append(file)
    if debug_print:
        print(_raytracing)
        print(_shaders)


def compile_rt():
    print(f"Compiling ({len(_raytracing)}) ray tracing shader(s)...")
    for r in _raytracing:
        command = f"glslangValidator -g -o {bin_dir}/{r}.spv -V {src_dir}/{r} --target-env {target_vk}"
        if debug_print:
            print(f"\t{command}")
        os.popen(command)


def compile_sh():
    print(f"Compiling ({len(_shaders)}) shader(s)...")
    for s in _shaders:
        command = f"glslc -o {bin_dir}/{s}.spv {src_dir}/{s}"
        if debug_print:
            print(f"\t{command}")
        os.popen(command)


def main():
    sort_shaders()
    compile_sh()
    if platform.system() != "Darwin":
        compile_rt()

main()
