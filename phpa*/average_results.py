#!/usr/bin/env python3
"""
Calculate average values from all result files in results/1, 2, 3, 4 directories.
"""

import os
import glob
from pathlib import Path
from collections import defaultdict

def parse_result_file(filepath):
    """Parse a single result file and extract metrics."""
    try:
        with open(filepath, 'r') as f:
            lines = f.readlines()
        
        metrics = {}
        for line in lines[:7]:  # Only first 7 lines contain data
            line = line.strip()
            if not line:
                continue
            
            if line.startswith("Path Length:"):
                metrics['path_length'] = int(line.split(":", 1)[1].strip())
            elif line.startswith("Max Memory:"):
                metrics['max_memory'] = float(line.split(":", 1)[1].strip().split()[0])
            elif line.startswith("Workers:"):
                metrics['workers'] = int(line.split(":", 1)[1].strip())
            elif line.startswith("Worker Max Memory:"):
                metrics['worker_max_memory'] = float(line.split(":", 1)[1].strip().split()[0])
            elif line.startswith("Path Cost:"):
                metrics['path_cost'] = float(line.split(":", 1)[1].strip())
            elif line.startswith("L1 Nodes:"):
                metrics['l1_nodes'] = int(line.split(":", 1)[1].strip())
            elif line.startswith("CPU Time:"):
                metrics['cpu_time'] = float(line.split(":", 1)[1].strip().split()[0])
        
        return metrics if len(metrics) == 7 else None
    except Exception as e:
        print(f"Error parsing {filepath}: {e}")
        return None

def main():
    """Find all result files and calculate averages."""
    script_dir = Path(__file__).parent
    results_dirs = [script_dir / f"results/{i}" for i in [1, 2, 3, 4]]
    
    # Collect all result files grouped by worker count
    results_by_workers = defaultdict(list)
    
    for results_dir in results_dirs:
        if not results_dir.exists():
            print(f"Directory not found: {results_dir}")
            continue
        
        result_files = sorted(results_dir.glob("result-*"))
        print(f"\nFound {len(result_files)} files in {results_dir}")
        
        for filepath in result_files:
            metrics = parse_result_file(filepath)
            if metrics:
                workers = metrics['workers']
                results_by_workers[workers].append(metrics)
    
    if not results_by_workers:
        print("No result files found!")
        return
    
    # Calculate and display averages for each worker count
    print("\n" + "="*80)
    print("AVERAGE RESULTS BY WORKER COUNT")
    print("="*80)
    
    for workers in sorted(results_by_workers.keys()):
        data = results_by_workers[workers]
        count = len(data)
        
        print(f"\nWorkers: {workers} (n={count} files)")
        print("-" * 80)
        
        # Calculate averages
        avg_path_length = sum(d['path_length'] for d in data) / count
        avg_max_memory = sum(d['max_memory'] for d in data) / count
        avg_worker_max_memory = sum(d['worker_max_memory'] for d in data) / count
        avg_path_cost = sum(d['path_cost'] for d in data) / count
        avg_l1_nodes = sum(d['l1_nodes'] for d in data) / count
        avg_cpu_time = sum(d['cpu_time'] for d in data) / count
        
        print(f"  Path Length:          {avg_path_length:>15.2f}")
        print(f"  Max Memory (MB):      {avg_max_memory:>15.4f}")
        print(f"  Worker Max Memory (MB): {avg_worker_max_memory:>15.4f}")
        print(f"  Path Cost:            {avg_path_cost:>15.2f}")
        print(f"  L1 Nodes:             {avg_l1_nodes:>15.2f}")
        print(f"  CPU Time (secs):      {avg_cpu_time:>15.6f}")
    
    # Overall averages across all files
    all_files = [m for metrics_list in results_by_workers.values() for m in metrics_list]
    total_count = len(all_files)
    
    if total_count > 1:
        print(f"\n{'='*80}")
        print(f"OVERALL AVERAGES (All {total_count} files)")
        print("="*80)
        
        print(f"  Path Length:          {sum(d['path_length'] for d in all_files) / total_count:>15.2f}")
        print(f"  Max Memory (MB):      {sum(d['max_memory'] for d in all_files) / total_count:>15.4f}")
        print(f"  Worker Max Memory (MB): {sum(d['worker_max_memory'] for d in all_files) / total_count:>15.4f}")
        print(f"  Path Cost:            {sum(d['path_cost'] for d in all_files) / total_count:>15.2f}")
        print(f"  L1 Nodes:             {sum(d['l1_nodes'] for d in all_files) / total_count:>15.2f}")
        print(f"  CPU Time (secs):      {sum(d['cpu_time'] for d in all_files) / total_count:>15.6f}")
        print("="*80)

if __name__ == "__main__":
    main()
