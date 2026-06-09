#!/usr/bin/env python3
"""
Benchmarking script for pathfinding algorithms (A*, HPA*, PHPA*) with varying CLUSTER_SIZE and WORKERS_SIZE.
Runs each algorithm multiple times for each CLUSTER_SIZE and WORKERS_SIZE configuration.
Modded to run PHPA/DHPA as a dual-process (Master/Worker) environment and extract worker performance metrics.
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
CLUSTER_SIZES = [
      15,
      20,
      25,
      35,
      40,
      45,
      50,
      60,
      70,
      80,
      90,
      100,
      110,
      120,
      130,
      140,
      150,
      160,
      170,
      180,
      190,
      200
    ]
WORKERS_SIZES = [1, 2, 4, 6, 8]
RUNS_PER_SIZE = 10
ALGORITHMS = ["dhpa"]
BASE_DIR = Path(__file__).parent.resolve()
CONSTANTS_FILE = BASE_DIR / "common" / "constants.h"
OUTPUT_FILE = BASE_DIR / "benchmark_results.json"
MASTER_TIMEOUT_SECONDS = 150

# Core static timing patterns (Mostly read from Master)
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
    
    algo_dir = BASE_DIR / algo
    if not algo_dir.exists():
        print(f"  ✗ {algo} directory not found")
        return False
    return build_dir_cmake_make(algo_dir)

def run_algorithm(algo: str, current_workers: int) -> dict:
    """Run an algorithm and extract timing data. Handles dual processes and captures worker data."""
    if algo == "dhpa":
        master_exe = BASE_DIR / "dhpa" / "master" / "build" / "master"
        worker_exe = BASE_DIR / "dhpa" / "worker" / "build" / "worker"
        
        if not master_exe.exists() or not worker_exe.exists():
            print(f"  ✗ Executables missing. Master: {master_exe.exists()}, Worker: {worker_exe.exists()}")
            return {}

        worker_processes = []
        try:
            for i in range(current_workers):
                # 1. START WORKERS
                proc = subprocess.Popen(
                    [str(worker_exe)],
                    cwd=worker_exe.parent,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True
                )
                worker_processes.append(proc)
            
                # 2. GIVE WORKERS A MOMENT TO INITIALIZE
                time.sleep(1.5)
                
                if proc.poll() is not None:
                    worker_stdout, worker_stderr = proc.communicate()
                    print(f"\n[WORKER {i} CRASHED ON STARTUP]")
                    print(f"--- Worker Stdout ---\n{worker_stdout}")
                    print(f"--- Worker Stderr ---\n{worker_stderr}\n---------------------")
                    return {}

            # 3. RUN MASTER 
            result = subprocess.run(
                [str(master_exe)],
                cwd=master_exe.parent,
                capture_output=True,
                text=True,
                timeout=MASTER_TIMEOUT_SECONDS
            )
            
            output = result.stdout + result.stderr

            if "Found overall path" not in output:
                print(f"\n[DEBUG] Master Output:\n{output}")

            # Extract metrics from Master output
            timings = {}
            for key, pattern in TIMING_PATTERNS.items():
                match = re.search(pattern, output)
                if match:
                    timings[key] = float(match.group(1))
            
            # Dynamic extraction for worker logs included in stdout/stderr output
            worker_metrics = {}
            
            # Find CPU times: "Worker X CPU time: Ys"
            cpu_matches = re.findall(r"Worker\s+(\d+)\s+CPU\s+time:\s+([\d.]+)s", output)
            for w_id, cpu_time in cpu_matches:
                w_key = f"worker_{w_id}_cpu_time"
                worker_metrics[w_key] = float(cpu_time)
                
            # Find Max Memory: "Worker X Max memory: Y MB"
            mem_matches = re.findall(r"Worker\s+(\d+)\s+Max\s+memory:\s+([\d.]+) \s*MB", output)
            for w_id, max_mem in mem_matches:
                w_key = f"worker_{w_id}_max_memory_mb"
                worker_metrics[w_key] = float(max_mem)
                
            # Merge worker analyses into main timings dict
            if worker_metrics:
                timings["worker_analysis"] = worker_metrics
            
            return timings

        except subprocess.TimeoutExpired:
            print(f"  ✗ Timeout running master process")
            if worker_processes:
                try:
                    w_out, w_err = worker_processes[0].communicate(timeout=0.5)
                    print(f"\n[TIMEOUT DEBUG] Worker 0 Stdout:\n{w_out}\nWorker 0 Stderr:\n{w_err}")
                except Exception:
                    pass
            return {}
        except Exception as e:
            print(f"  ✗ Error running DHPA processes: {e}")
            return {}
        finally:
            # Cleanly teardown ALL active worker processes
            for proc in worker_processes:
                if proc.poll() is None:
                    proc.terminate()
            
            for proc in worker_processes:
                if proc.poll() is None:
                    try:
                        proc.wait(timeout=2.0)
                    except subprocess.TimeoutExpired:
                        proc.kill()
            
            time.sleep(1.0)
                    
    else:
        # Legacy fallback logic for single executable scripts
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
    print("Pathfinding Algorithm Benchmark (Multi-Process / Multi-Worker DHPA)")
    print("=" * 70)
    print(f"Cluster sizes : {CLUSTER_SIZES}")
    print(f"Workers sizes : {WORKERS_SIZES}")
    print(f"Runs per size : {RUNS_PER_SIZE}")
    print(f"Algorithms    : {', '.join(ALGORITHMS)}")
    print(f"Results File  : {OUTPUT_FILE}")
    print("=" * 70)
    
    results = {
        "metadata": {
            "timestamp": datetime.now().isoformat(),
            "cluster_sizes": CLUSTER_SIZES,
            "workers_sizes": WORKERS_SIZES,
            "runs_per_size": RUNS_PER_SIZE,
            "algorithms": ALGORITHMS,
        },
        "data": []
    }
    
    for workers_count in WORKERS_SIZES:
        print(f"\n=========================================")
        print(f"=== TESTING WITH WORKERS_SIZE = {workers_count} ===")
        print(f"=========================================")
        
        # Modify workers count configuration
        modify_workers_count(workers_count)
        
        for cluster_size in CLUSTER_SIZES:
            print(f"\n--- CLUSTER_SIZE = {cluster_size} | WORKERS = {workers_count} ---")
            
            # Modify constants.h for cluster size
            modify_cluster_size(cluster_size)
            
            # Recompile algorithms with the updated constants
            print("  Building algorithms...")
            build_failed = False
            for algo in ALGORITHMS:
                if not build_algorithm(algo):
                    print(f"  Warning: Could not build {algo}, skipping configuration loop")
                    build_failed = True
                    break
            
            if build_failed:
                continue
            
            # Execute runs
            for algo in ALGORITHMS:
                print(f"  Running {algo} ({RUNS_PER_SIZE}x)...")
                
                for run in range(RUNS_PER_SIZE):
                    timings = run_algorithm(algo, current_workers=workers_count)
                    
                    if timings:
                        data_point = {
                            "cluster_size": cluster_size,
                            "workers_size": workers_count,
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