# Docker Setup Guide for Distributed HPA*

## Quick Start

### Run with docker-compose (recommended)
```bash
# Start master + 3 workers
docker-compose up -d

# View logs
docker-compose logs -f

# Stop all services
docker-compose down
```

### Build the Docker image manually
```bash
sudo docker build -t hpa-distributed .
```

## Running Individual Services

### Master only
```bash
sudo docker run --rm -p 9090:9090 \
  -v $(pwd)/data:/app/data:ro \
  -v $(pwd)/results:/app/results \
  hpa-distributed ./master/mphpa
```

### Single worker (connecting to master on host machine)
```bash
sudo docker run --rm \
  -v $(pwd)/data:/app/data:ro \
  -e MASTER_HOST=host.docker.internal \
  -e MASTER_PORT=9090 \
  hpa-distributed ./worker/wphpa
```

### Multiple workers with docker-compose
The included `docker-compose.yml` runs master + 3 workers in a network. Scale workers:
```bash
# Start with 5 workers instead of 3
docker-compose up -d --scale worker-1=5
```

## Environment Variables

- **MASTER_HOST**: Hostname/IP of master server
  - Docker containers: use service name `master`
  - Local machine: use `host.docker.internal`
  - Default: `127.0.0.1`

- **MASTER_PORT**: TCP port for master
  - Default: `9090`

## Volume Mounts

- **Data**: `/app/data` - Map files (read-only)
- **Results**: `/app/results` - Output files (read-write)

## Networking

When using `docker-compose`:
- All services connect via internal bridge network `hpa-network`
- Master exposed on `localhost:9090` for external access
- Workers communicate internally via `master` hostname

## Troubleshooting

### Master not accessible from outside
Check that port 9090 is exposed in docker-compose.yml:
```yaml
ports:
  - "9090:9090"
```

### Workers can't find master
Verify hostname in MASTER_HOST environment variable. For docker-compose, use `master`, not `localhost`.

### Permission errors with volumes
Ensure data directory is readable and results directory is writable:
```bash
chmod 755 data
chmod 755 results
```

### Build errors
Ensure Docker has enough memory (default 2GB may be tight for C build):
```bash
docker build --memory=4g -t hpa-distributed .
```

## Development Workflow

### Rebuild after code changes
```bash
docker-compose build
docker-compose up
```

### Interactive debugging
```bash
docker run -it --rm \
  -v $(pwd):/app \
  ubuntu:22.04 bash
# Inside container: manually compile and debug
```

### Run benchmark
```bash
docker run --rm \
  -v $(pwd):/app \
  -w /app \
  hpa-distributed \
  python3 benchmark.py
```
