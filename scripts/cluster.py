#!/bin/python3

import signal
import sys
import subprocess
import importlib.util
import colorama
import pty
import os

next_cluster_executable_path = "bin/release/next_cluster"
make_states_executable_path = "bin/release/make_states"
simulate_many_executable_path = "bin/release/simulate_many"

width=1000
height=1000
col=500
row=500
gen_limit="1'000'000'000"
out_dir = None
sim_count = None

def ctrl_c_handler(sig, frame):
    print('')
    print('Terminating...')
    sys.exit(1)

if (__name__ == "__main__"):
  signal.signal(signal.SIGINT, ctrl_c_handler)

  spec = importlib.util.find_spec("colorama")
  if spec is None:
    raise ImportError("Package 'colorama' is not installed. Please install it using 'pip install colorama'")

  colorama.init(autoreset=True)

  if (len(sys.argv) == 1):
    print(f"Usage: {sys.argv[0]} <out_dir> <sim_count>")
    sys.exit(1)

  out_dir = sys.argv[1]
  sim_count = sys.argv[2]

  proc = subprocess.Popen([next_cluster_executable_path, out_dir],
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE)

  # Wait for the subprocess to finish and capture the exit code
  next_cluster_exit_code = proc.wait()
  cluster = next_cluster_exit_code

  print(f"Cluster {cluster}, {out_dir}")

  print("[[ make_states ]]")

  master_fd, slave_fd = pty.openpty()

  proc = subprocess.Popen(
    [
      make_states_executable_path,
      "-N", f"{sim_count}",
      "-o", f"{out_dir}/cluster{cluster}",
      "-n", "randwords,2",
      "-m", "256", "-M", "256",
      "-t", "LR",
      "-s", "rand",
      "-w", f"{width}", "-h", f"{height}",
      "-x", f"{col}", "-y", f"{row}",
      "-O", "NESW",
      "-g", "fill=0",
      "-W", "resources/words.txt",
      "-c",
    ],
    stdout=slave_fd, stderr=slave_fd, text=True,
    bufsize=1, close_fds=True, universal_newlines=True
  )

  os.close(slave_fd)

  # Process the output in real-time
  while True:
      try:
          output = os.read(master_fd, 1024).decode()
      except OSError:
          break
      if not output:
          break
      print(output.strip())

  make_states_exit_code = proc.wait()
  os.close(master_fd)

  if (make_states_exit_code != 0):
    print(f"make_states exited with code {make_states_exit_code}, terminating!")
    colorama.deinit()
    exit(1)

  print("[[ simulate_many ]]")

  master_fd, slave_fd = pty.openpty()

  proc = subprocess.Popen(
    [
      simulate_many_executable_path,
      # "-T" "6"
      "-Q", "50",
      "-L", f"{out_dir}/cluster{cluster}/log.txt",
      "-S", f"{out_dir}/cluster{cluster}",
      "-g", f"{gen_limit}",
      "-f", "raw",
      "-o", f"{out_dir}/cluster{cluster}",
      "-s", "-y", "-l", "-C"
    ],
    stdout=slave_fd, stderr=slave_fd, text=True,
    bufsize=1, close_fds=True, universal_newlines=True
  )

  os.close(slave_fd)

  # Process the output in real-time
  while True:
      try:
          output = os.read(master_fd, 1024).decode()
      except OSError:
          break
      if not output:
          break
      print(output.strip())

  simulate_many_exit_code = proc.wait()
  os.close(master_fd)

  if (simulate_many_exit_code != 0):
    print(f"simulate_many exited with code {simulate_many_exit_code}")

  colorama.deinit()
