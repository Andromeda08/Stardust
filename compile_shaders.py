import io
import os
import glob
import platform
import subprocess

# shader source file extensions
rt_extensions = ["rchit", "rgen", "rmiss"]
sh_extensions = ["vert", "frag", "glsl", "geom", "comp"]

# configuration
debug_print = True

src_dir = "Resources/Shaders"
bin_dir = f"{src_dir}/out"
target_vk = "vulkan1.3"

# List of shaders to compile
_to_compile = []


def sort_shaders():
    all_shaders = glob.glob(f"{src_dir}/*")
    for shader in all_shaders:
        ext = shader.rsplit('.')[-1]
        file = shader.rsplit('\\')[-1]

        if ext in sh_extensions:
            _to_compile.append(file)

        if platform.system() != "Darwin" and ext in rt_extensions:
            _to_compile.append(file)

    if debug_print:
        print(_to_compile)


def compile_shaders():
    print(f"Compiling ({len(_to_compile)}) shaders...")
    for shader in _to_compile:
        file = shader.rsplit('\\')[-1]
        command = f"glslangValidator -g -o {bin_dir}/{file}.spv -V {src_dir}/{file} --target-env vulkan1.3"
        with subprocess.Popen(command, stdout=subprocess.PIPE, shell=True) as proc:
            for line in io.TextIOWrapper(proc.stdout, encoding="utf-8"):
                print(line.rstrip())


if not os.path.exists(f"{bin_dir}"):
    os.makedirs(f"{bin_dir}")

sort_shaders()
compile_shaders()
