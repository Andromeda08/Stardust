import io
import os
import glob
import subprocess

rt_extensions = ["rchit", "rgen", "rmiss"]
path = "Resources/Shaders/Reflection"
_shaders = []

if not os.path.exists(f"{path}/out"):
    os.makedirs(f"{path}/out")
shaders = glob.glob(f"{path}/*")

for shader in shaders:
    if shader.rsplit('.')[-1] in rt_extensions:
        _shaders.append(shader.rsplit('\\')[-1])

print(f"Compiling ({len(_shaders)}) raytracing shaders...")
for shader in _shaders:
    if shader.rsplit('.')[-1] in rt_extensions:
        file = shader.rsplit('\\')[-1]
        command = f"glslangValidator -g -o {path}/out/{file}.spv -V {path}/{file} --target-env vulkan1.3"
        with subprocess.Popen(command, stdout=subprocess.PIPE, shell=True) as proc:
            for line in io.TextIOWrapper(proc.stdout, encoding="utf-8"):
                print(line.rstrip())
