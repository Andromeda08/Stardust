import io
import os
import glob
import subprocess

shaders = []
bin_dir = "bin"
target_env = "vulkan1.3"


def collect_shaders():
    for shader_lang in ["hlsl", "glsl"]:
        shaders_in_dir = glob.glob(f"{shader_lang}/*")
        for sh in shaders_in_dir:
            sh_file = sh.rsplit("\\")[-1]
            if "." in sh:
                shaders.append(f"{shader_lang}/{sh_file}")


def compile_shaders():
    collect_shaders()
    if not os.path.exists(f"{bin_dir}"):
        os.makedirs(f"{bin_dir}")

    counter = 0
    print(f"Compiling {len(shaders)} shader(s), target environment: {target_env}")
    for sh in shaders:
        command = f"glslangValidator -g -o {bin_dir}/{sh.rsplit('/')[-1]}.spv -V {sh} --target-env {target_env}"
        if "hlsl" in sh:
            command += " -e main"

        with subprocess.Popen(command, stdout=subprocess.PIPE, shell=True) as proc:
            for line in io.TextIOWrapper(proc.stdout, encoding="utf-8"):
                print(line.rstrip())


compile_shaders()
