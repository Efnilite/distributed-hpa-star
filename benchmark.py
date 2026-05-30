#!/usr/bin/env python3
"""
Benchmarking script for pathfinding algorithms (A*, HPA*, PHPA*) with varying CLUSTER_SIZE.
Runs each algorithm 10 times for each CLUSTER_SIZE value and collects timing data.
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
CLUSTER_SIZES = [100]  # 1 to 300 with steps of 10
RUNS_PER_SIZE = 50
ALGORITHMS = ["hpa"]  # "phpa" can be added if needed
BASE_DIR = Path(__file__).parent.resolve()
CONSTANTS_FILE = BASE_DIR / "common" / "constants.h"
OUTPUT_FILE = BASE_DIR / "benchmark_results.json"

# Timing pattern to extract from algorithm output
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
    
    # Replace the CLUSTER_SIZE definition
    new_content = re.sub(
        r"#define CLUSTER_SIZE \d+",
        f"#define CLUSTER_SIZE {size}",
        content
    )
    
    with open(CONSTANTS_FILE, "w") as f:
        f.write(new_content)
    
    print(f"  ✓ Set CLUSTER_SIZE to {size}")

def build_algorithm(algo: str) -> bool:
    """Build an algorithm. Returns True if successful."""
    algo_dir = None
    
    if algo == "a":
        algo_dir = BASE_DIR / "a"
    elif algo == "hpa":
        algo_dir = BASE_DIR / "hpa"
    elif algo == "phpa":
        algo_dir = BASE_DIR / "phpa"
    
    if not algo_dir or not algo_dir.exists():
        print(f"  ✗ {algo} directory not found")
        return False
    
    build_dir = algo_dir
    
    # Create build directory if it doesn't exist
    if not build_dir.exists():
        build_dir.mkdir(parents=True, exist_ok=True)
    
    # Run CMake and build
    try:
        subprocess.run(
            ["cmake", "-DCMAKE_BUILD_TYPE=Release", "."],
            cwd=build_dir,
            capture_output=True,
            check=True,
            timeout=30
        )
        subprocess.run(
            ["make", "-j"],
            cwd=build_dir,
            capture_output=True,
            check=True,
            timeout=60
        )
        print(f"  ✓ Built {algo}")
        return True
    except subprocess.CalledProcessError as e:
        print(f"  ✗ Failed to build {algo}")
        print(f"    stdout: {e.stdout.decode()}")
        print(f"    stderr: {e.stderr.decode()}")
        return False
    except subprocess.TimeoutExpired:
        print(f"  ✗ Build timeout for {algo}")
        return False

def run_algorithm(algo: str) -> dict:
    """Run an algorithm and extract timing data. Returns dict of timings."""
    algo_dir = None
    exe_name = algo
    
    if algo == "a":
        algo_dir = BASE_DIR / "a*"
    elif algo == "hpa":
        algo_dir = BASE_DIR / "hpa*"
    elif algo == "phpa":
        algo_dir = BASE_DIR / "phpa*"
        exe_name = "phpa"  # Adjust if needed
    
    exe_path = algo_dir / exe_name
    
    if not exe_path.exists():
        print(f"  ✗ Executable not found: {exe_path}")
        return {}
    
    try:
        result = subprocess.run(
            [str(exe_path)],
            cwd=algo_dir,
            capture_output=True,
            text=True,
            timeout=30
        )
        
        output = result.stdout + result.stderr
        
        # Extract timing data
        timings = {}
        for key, pattern in TIMING_PATTERNS.items():
            match = re.search(pattern, output)
            if match:
                timings[key] = float(match.group(1))
        
        return timings
    except subprocess.TimeoutExpired:
        print(f"  ✗ Timeout running {algo}")
        return {}
    except Exception as e:
        print(f"  ✗ Error running {algo}: {e}")
        return {}

def run_benchmark():
    """Run the complete benchmark."""
    print("=" * 70)
    print("Pathfinding Algorithm Benchmark")
    print("=" * 70)
    print(f"Cluster sizes: {CLUSTER_SIZES[0]} to {CLUSTER_SIZES[-1]} (step 10)")
    print(f"Runs per size: {RUNS_PER_SIZE}")
    print(f"Algorithms: {', '.join(ALGORITHMS)}")
    print(f"Results will be saved to: {OUTPUT_FILE}")
    print("=" * 70)
    
    results = {
        "metadata": {
            "timestamp": datetime.now().isoformat(),
            "cluster_sizes": CLUSTER_SIZES,
            "runs_per_size": RUNS_PER_SIZE,
            "algorithms": ALGORITHMS,
        },
        "data": []
    }
    
    total_iterations = len(CLUSTER_SIZES) * len(ALGORITHMS) * RUNS_PER_SIZE
    current_iteration = 0
    
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
                    
                    # Print progress
                    current_iteration += 1
                    overall_path = timings.get("found_overall_path", 0)
                    print(f"    Run {run + 1}/{RUNS_PER_SIZE}: {overall_path:.6f}s overall", end="\r")
                else:
                    current_iteration += 1
                    print(f"    Run {run + 1}/{RUNS_PER_SIZE}: FAILED", end="\r")
            
            print()  # New line after runs
    
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
