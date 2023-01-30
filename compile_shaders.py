import io
import os
import glob
import platform
import subprocess

# shader source file extensions
rt_extensions = ["rchit", "rgen", "rmiss"]
sh_extensions = ["vert", "frag", "geom", "comp"]

# configuration
debug_print = False

src_dirs = ["Resources/Shaders", "Resources/Raytracing"]
bin_dir = "cmake-build-debug"
target_vk = "vulkan1.3"
slash = "\\"

# List of shaders to compile
_to_compile = []


def sort_shaders():
    for src_dir in src_dirs:
        all_shaders = glob.glob(f"{src_dir}/*")
        for shader in all_shaders:
            ext = shader.rsplit('.')[-1]
            file = shader.rsplit(slash)[-1]

            if ext in sh_extensions:
                _to_compile.append(f"{src_dir}/{file}")

            if platform.system() != "Darwin" and ext in rt_extensions:
                _to_compile.append(f"{src_dir}/{file}")

    if debug_print:
        print(_to_compile)


def compile_shaders():
    print(f"Compiling ({len(_to_compile)}) shaders...\nTarget environment: {target_vk}")
    counter = 0
    for shader in _to_compile:
        command = f"glslangValidator -g -o {bin_dir}/{shader.rsplit('/')[-1]}.spv -V {shader} --target-env {target_vk}"
        with subprocess.Popen(command, stdout=subprocess.PIPE, shell=True) as proc:
            for line in io.TextIOWrapper(proc.stdout, encoding="utf-8"):
                counter += 1
                print(f"({ counter }) {line.rstrip()}")


if not os.path.exists(f"{bin_dir}"):
    os.makedirs(f"{bin_dir}")

sort_shaders()
compile_shaders()
