from tqdm import tqdm
import os
import subprocess
from datetime import datetime
import shutil

############ CONSTANT ###########

RESULTS_PATH = "tmp/results"

BUILD_COMMANDS = ["cmake -DCMAKE_BUILD_TYPE=RELEASE ..", "make"]
TEST_EXECS = ["./test_trie", "./test_levenshtein"]
BENCH_EXECS = ["time_trie", "time_levenshtein", "time_levenshtein_query"]

############# BUILD #############
os.makedirs("build", exist_ok=True)

with tqdm(total=len(BUILD_COMMANDS), desc="Building") as pbar:
    for command in BUILD_COMMANDS:
        p = subprocess.run(command.split(" "), cwd="build", capture_output=True)
        pbar.write(p.stdout.decode())
        p.check_returncode()
        pbar.update(1)


############# EXEC TESTS #############
with tqdm(total=len(TEST_EXECS), desc="Testing") as pbar:
    for execs in TEST_EXECS:
        p = subprocess.run([execs], cwd="build", capture_output=True)
        pbar.write(p.stdout.decode())
        p.check_returncode()
        pbar.update(1)

############ PREPARE RESULT DIR ###########
time_stamp = datetime.now().strftime("%Y_%m_%d_%H_%M_%S")
result_path = f"{RESULTS_PATH}/{time_stamp}"
os.makedirs(result_path, exist_ok=True)

############ SAVE IMPL ###########
shutil.copytree("implementation", f"{result_path}/implementation")
shutil.copytree("source", f"{result_path}/source")

for file in os.listdir("eval"):
    shutil.copy2(f"eval/{file}", result_path)

############# RUN BENCHES ############
with tqdm(total=len(BENCH_EXECS), desc="Benchmark") as pbar:
    for execs in BENCH_EXECS:

        with subprocess.Popen(
            [
                f"./{execs}",
                "-o",
                f"../{result_path}/{execs}.txt",
            ],
            cwd="build",
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True
        ) as p:
            for line in p.stdout:  # type: ignore
                pbar.write(line.strip())
            p.wait()

        pbar.update(1)

############ RUN PLOT SCRIPTS ##########
os.chdir(result_path)
for file in os.listdir():
    if file.endswith(".py"):
        p = subprocess.run(["python", file], capture_output=True)
        if (p.returncode != 0):
            print(f"{file} failed! + {p.stdout.decode()}")

print("All finished. Bye!")
