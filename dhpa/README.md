# Build images
sudo docker-compose build

# Run master
sudo docker-compose run master

# Run worker
sudo docker-compose run worker

# Run both in the background
sudo docker-compose up -d

# Stop containers
sudo docker-compose down