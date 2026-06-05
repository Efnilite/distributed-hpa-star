#!/usr/bin/env python3
"""
Benchmarking script for pathfinding algorithms (A*, HPA*, PHPA*) with varying CLUSTER_SIZE and WORKERS_SIZE.
Runs each algorithm 10 times for each CLUSTER_SIZE value and collects timing data.
Modded to run PHPA/DHPA as a dual-process (Master/Worker) environment.
"""

import os
import re
import subprocess
import json
import sys
from pathlib import Path
from datetime import datetime
import time

# Configuration
CLUSTER_SIZES = [15, 20, 25, 35, 40, 45, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200]
# CLUSTER_SIZES = [10]
NUM_WORKERS = 1
RUNS_PER_SIZE = 10
ALGORITHMS = ["dhpa"]  
BASE_DIR = Path(__file__).parent.resolve()
CONSTANTS_FILE = BASE_DIR / "common" / "constants.h"
OUTPUT_FILE = BASE_DIR / "benchmark_results.json"
MASTER_TIMEOUT_SECONDS = 150

# Timing pattern to extract from algorithm output (Usually read from Master)
TIMING_PATTERNS = {
    "inter_edges": r"Find inter edges - ([\d.]+)s",
    "preprocessed": r"Find intra cluster paths - ([\d.]+)s",
    "finish_abstract_graph": r"Finish abstract graph for search - ([\d.]+)s",
    "found_graph_path": r"Found graph path - ([\d.]+)s",
    "found_overall_path": r"Found overall path - ([\d.]+)s",
    "max_memory": r"Max memory: ([\d.]+) MB",
    "path_length": r"Path length: ([\d.]+)",
}

def modify_cluster_size(size: int) -> None:
    """Modify CLUSTER_SIZE in constants.h"""
    with open(CONSTANTS_FILE, "r") as f:
        content = f.read()
    
    new_content = re.sub(
        r"#define CLUSTER_SIZE \d+",
        f"#define CLUSTER_SIZE {size}",
        content
    )
    
    with open(CONSTANTS_FILE, "w") as f:
        f.write(new_content)
    
    print(f"  ✓ Set CLUSTER_SIZE to {size}")

def modify_workers_count(count: int) -> None:
    """Modify WORKERS_SIZE in constants.h"""
    with open(CONSTANTS_FILE, "r") as f:
        content = f.read()
    
    # Replace the WORKERS_SIZE definition
    new_content = re.sub(
        r"#define WORKERS_SIZE \d+",
        f"#define WORKERS_SIZE {count}",
        content
    )
    
    with open(CONSTANTS_FILE, "w") as f:
        f.write(new_content)
    
    print(f"  ✓ Set WORKERS_SIZE to {count}")

def build_dir_cmake_make(build_dir: Path) -> bool:
    """Helper to run CMake and Make inside a specified directory."""
    if not build_dir.exists():
        build_dir.mkdir(parents=True, exist_ok=True)
    try:
        subprocess.run(
            ["cmake", "-DCMAKE_BUILD_TYPE=Release", "."],
            cwd=build_dir, capture_output=True, check=True, timeout=30
        )
        subprocess.run(
            ["make", "-j"],
            cwd=build_dir, capture_output=True, check=True, timeout=60
        )
        return True
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as e:
        print(f"    ✗ Build error in {build_dir.name}")
        if hasattr(e, 'stdout') and e.stdout:
            print(f"    stdout: {e.stdout.decode()}")
        if hasattr(e, 'stderr') and e.stderr:
            print(f"    stderr: {e.stderr.decode()}")
        return False

def build_algorithm(algo: str) -> bool:
    """Build an algorithm. Returns True if successful."""
    if algo == "dhpa":
        print("    Building DHPA Master...")
        master_success = build_dir_cmake_make(BASE_DIR / "dhpa" / "master" / "build")
        print("    Building DHPA Worker...")
        worker_success = build_dir_cmake_make(BASE_DIR / "dhpa" / "worker" / "build")
        return master_success and worker_success
    
    # Legacy handler for mono-process algorithms
    algo_dir = BASE_DIR / algo
    if not algo_dir.exists():
        print(f"  ✗ {algo} directory not found")
        return False
    return build_dir_cmake_make(algo_dir)

def run_algorithm(algo: str) -> dict:
    """Run an algorithm and extract timing data. Handles dual processes for dhpa."""
    if algo == "dhpa":
        master_exe = BASE_DIR / "dhpa" / "master" / "build" / "master"
        worker_exe = BASE_DIR / "dhpa" / "worker" / "build" / "worker"
        
        if not master_exe.exists() or not worker_exe.exists():
            print(f"  ✗ Executables missing. Master: {master_exe.exists()}, Worker: {worker_exe.exists()}")
            return {}

        worker_proc = None
        try:
            # 1. START WORKER FIRST
            worker_proc = subprocess.Popen(
                [str(worker_exe)],
                cwd=worker_exe.parent,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            
            # 2. GIVE WORKER A MOMENT TO INITIALIZE & BIND PORTS
            time.sleep(1.5)
            
            # Check if the worker crashed immediately on startup
            if worker_proc.poll() is not None:
                worker_stdout, worker_stderr = worker_proc.communicate()
                print(f"\n[WORKER CRASHED ON STARTUP]")
                print(f"--- Worker Stdout ---\n{worker_stdout}")
                print(f"--- Worker Stderr ---\n{worker_stderr}\n---------------------")
                return {}

            # 3. RUN MASTER (Blocking process that collects data)
            result = subprocess.run(
                [str(master_exe)],
                cwd=master_exe.parent,
                capture_output=True,
                text=True,
                timeout=MASTER_TIMEOUT_SECONDS
            )
            
            output = result.stdout + result.stderr

            # DEBUG: Print Master output if timing data is completely missing
            if "Found overall path" not in output:
                print(f"\n[DEBUG] Master Output:\n{output}")

            # Extract metrics from Master output
            timings = {}
            for key, pattern in TIMING_PATTERNS.items():
                match = re.search(pattern, output)
                if match:
                    timings[key] = float(match.group(1))
            
            return timings

        except subprocess.TimeoutExpired:
            print(f"  ✗ Timeout running master process")
            if worker_proc:
                try:
                    w_out, w_err = worker_proc.communicate(timeout=0.5)
                    print(f"\n[TIMEOUT DEBUG] Worker Stdout:\n{w_out}\nWorker Stderr:\n{w_err}")
                except Exception:
                    pass
            return {}
        except Exception as e:
            print(f"  ✗ Error running DHPA processes: {e}")
            return {}
        finally:
            # 5. TEARDOWN WORKER Cleanly
            if worker_proc and worker_proc.poll() is None:
                worker_proc.terminate()
                try:
                    worker_proc.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    worker_proc.kill()
            
            # Small cooldown so the OS can cleanly free up local network ports/sockets
            time.sleep(1.0)
                    
    else:
        # Legacy fallback logic for single executable scripts (e.g., A* or HPA*)
        exe_path = BASE_DIR / algo / algo
        if not exe_path.exists():
            return {}
        try:
            result = subprocess.run([str(exe_path)], cwd=exe_path.parent, capture_output=True, text=True, timeout=30)
            output = result.stdout + result.stderr
            timings = {}
            for key, pattern in TIMING_PATTERNS.items():
                match = re.search(pattern, output)
                if match: timings[key] = float(match.group(1))
            return timings
        except Exception:
            return {}

def run_benchmark():
    """Run the complete benchmark."""
    print("=" * 70)
    print("Pathfinding Algorithm Benchmark (Multi-Process DHPA Enabled)")
    print("=" * 70)
    print(f"Cluster sizes: {CLUSTER_SIZES[0]} to {CLUSTER_SIZES[-1]} (step 10)")
    print(f"Workers size : {NUM_WORKERS}")
    print(f"Runs per size: {RUNS_PER_SIZE}")
    print(f"Algorithms: {', '.join(ALGORITHMS)}")
    print(f"Results will be saved to: {OUTPUT_FILE}")
    print("=" * 70)
    
    results = {
        "metadata": {
            "timestamp": datetime.now().isoformat(),
            "cluster_sizes": CLUSTER_SIZES,
            "workers_size": NUM_WORKERS,
            "runs_per_size": RUNS_PER_SIZE,
            "algorithms": ALGORITHMS,
        },
        "data": []
    }
    
    # Modify workers size once globally before looping over cluster sizes
    modify_workers_count(NUM_WORKERS)
    
    for cluster_size in CLUSTER_SIZES:
        print(f"\n--- CLUSTER_SIZE = {cluster_size} ---")
        
        # Modify constants.h
        modify_cluster_size(cluster_size)
        
        # Build all algorithms
        print("  Building algorithms...")
        for algo in ALGORITHMS:
            if not build_algorithm(algo):
                print(f"  Warning: Could not build {algo}, skipping runs")
                continue
        
        # Run each algorithm multiple times
        for algo in ALGORITHMS:
            print(f"  Running {algo} ({RUNS_PER_SIZE}x)...")
            
            for run in range(RUNS_PER_SIZE):
                timings = run_algorithm(algo)
                
                if timings:
                    data_point = {
                        "cluster_size": cluster_size,
                        "algorithm": algo,
                        "run": run + 1,
                        "timings": timings,
                    }
                    results["data"].append(data_point)
                    
                    overall_path = timings.get("found_overall_path", 0)
                    print(f"    Run {run + 1}/{RUNS_PER_SIZE}: {overall_path:.6f}s overall", end="\r")
                else:
                    print(f"    Run {run + 1}/{RUNS_PER_SIZE}: FAILED", end="\r")
            print() 
    
    # Save results to JSON
    print("\n" + "=" * 70)
    print(f"Saving results to {OUTPUT_FILE}...")
    with open(OUTPUT_FILE, "w") as f:
        json.dump(results, f, indent=2)
    
    print(f"✓ Benchmark complete! Results saved to {OUTPUT_FILE}")
    print(f"Total data points collected: {len(results['data'])}")

if __name__ == "__main__":
    try:
        run_benchmark()
    except KeyboardInterrupt:
        print("\n\nBenchmark interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n\nError during benchmark: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)