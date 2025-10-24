import subprocess
import signal
import sys




processes = []

def run():
    try:
        for _ in range(0, 100):
            for core in range(0, 32):
                addr = 0
                if core == 2:
                    addr = 0xfd790000
                else:
                    addr = 0xfd790000 + (1 + core) * 0x1000

                cmd = ["sudo", "taskset", "-c", str(core), "./um", "addr", str(hex(addr))]
                p = subprocess.Popen(cmd)
                processes.append(p)
    except KeyboardInterrupt:
        print("Stopping all processes...")
        for p in processes:
            p.terminate()
        for p in processes:
            p.wait()
        sys.exit(0)

if __name__ == "__main__":
    run()
