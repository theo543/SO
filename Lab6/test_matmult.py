import numpy as np
import subprocess
import sys

MAX_VALUE = 1_000_000
MAX_DIMENSION = 80

def mat_to_file(mat, fd):
    print(f"{np.size(mat, axis=0)} {np.size(mat, axis=1)}\n", file=fd)
    np.savetxt(fd, mat, fmt="%i")

def one_test(location) -> bool:
    if len(sys.argv) > 2:
        print(f"Fowarding arguments {sys.argv[2:]}")
    gen = np.random.default_rng()
    [m, p, n] = gen.integers(1, MAX_DIMENSION, size=3)
    print(f"Testing {m}x{p} matrix times {p}x{n} matrix:")
    mat_a = gen.integers(1, MAX_VALUE, size=(m, p), dtype=np.int64)
    mat_b = gen.integers(1, MAX_VALUE, size=(p, n), dtype=np.int64)
    mat_out = np.matmul(mat_a, mat_b)
    proc = subprocess.Popen([location] + sys.argv[2:], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=sys.stderr.fileno(), encoding='utf-8')
    stdin = proc.stdin
    stdout = proc.stdout
    mat_to_file(mat_a, stdin)
    mat_to_file(mat_b, stdin)
    stdin.flush()
    sizes = stdout.readline().split(' ')
    out = np.loadtxt(stdout, dtype=np.int64)
    assert(proc.wait() == 0)
    good_sizes = (m, n)
    out_sizes = (int(sizes[0]), int(sizes[1]))
    if good_sizes != out_sizes:
        print(f"Excepted size {good_sizes}, got {out_sizes}")
        return False
    # readtxt seems to return column vector instead of row vector sometimes?
    out = np.reshape(out, good_sizes)
    if not (np.all(mat_out == out)):
        print("Wrong result")
        print(f"Expected {mat_out}\nGot {out}")      
        return False
    print("Success!\n")
    return True

def main():
    while one_test(sys.argv[1]):
        pass

if __name__ == "__main__":
    main()
