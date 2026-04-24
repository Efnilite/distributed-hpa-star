#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define WALL '@'
#define PATH '.'

typedef struct
{
    int x, y;
} Point;

// Get random direction (0=up, 1=right, 2=down, 3=left)
int get_random_direction()
{
    return rand() % 4;
}

// Check if a cell is within bounds
int is_within_bounds(const int x, const int y, const int width, const int height)
{
    return x > 0 && x < width - 1 && y > 0 && y < height - 1;
}

// Check if a cell is a path (already visited)
int is_path(char** maze, const int x, const int y, const int width, const int height)
{
    if (!is_within_bounds(x, y, width, height))
        return 0;
    return maze[y][x] == PATH;
}

// Iterative backtracking maze generation using explicit stack
void generate_maze_iterative(char** maze, const int start_x, const int start_y, const int width, const int height)
{
    // Allocate stack for iterative backtracking
    Point* stack = malloc(width * height * sizeof(Point));
    int stack_size = 0;

    // Push starting position
    stack[stack_size].x = start_x;
    stack[stack_size].y = start_y;
    stack_size++;
    maze[start_y][start_x] = PATH;

    // Directions: up, right, down, left
    const int dx[] = {0, 1, 0, -1};
    const int dy[] = {-1, 0, 1, 0};

    while (stack_size > 0)
    {
        const int x = stack[stack_size - 1].x;
        const int y = stack[stack_size - 1].y;

        // Randomize directions
        int directions[] = {0, 1, 2, 3};
        for (int i = 3; i > 0; i--)
        {
            const int j = rand() % (i + 1);
            const int temp = directions[i];
            directions[i] = directions[j];
            directions[j] = temp;
        }

        int found_unvisited = 0;
        for (int i = 0; i < 4; i++)
        {
            const int dir = directions[i];
            const int nx = x + dx[dir] * 2;
            const int ny = y + dy[dir] * 2;

            if (is_within_bounds(nx, ny, width, height) && maze[ny][nx] == WALL)
            {
                // Carve the wall between current and next cell
                maze[y + dy[dir]][x + dx[dir]] = PATH;
                maze[ny][nx] = PATH;

                // Push next cell onto stack
                stack[stack_size].x = nx;
                stack[stack_size].y = ny;
                stack_size++;

                found_unvisited = 1;
                break;
            }
        }

        // Backtrack if no unvisited neighbors
        if (!found_unvisited)
        {
            stack_size--;
        }
    }

    free(stack);
}

int main(int argc, char* argv[])
{
    int width = 101;
    int height = 101;

    if (argc == 3)
    {
        width = atoi(argv[1]);
        height = atoi(argv[2]);

        if (width % 2 == 0)
        {
            width++;
        }
        if (height % 2 == 0)
        {
            height++;
        }
    }

    char** maze = malloc(height * sizeof(char*));
    if (maze == NULL)
    {
        perror("Failed to malloc maze");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < height; i++)
    {
        maze[i] = (char*)malloc((width + 1) * sizeof(char));
        memset(maze[i], WALL, width);
        maze[i][width] = '\0';
    }

    srand(time(NULL));
    generate_maze_iterative(maze, 1, 1, width, height);

    for (int i = 0; i < width; i++)
    {
        maze[0][i] = WALL;
        maze[height - 1][i] = WALL;
    }
    for (int i = 0; i < height; i++)
    {
        maze[i][0] = WALL;
        maze[i][width - 1] = WALL;
    }

    FILE* file = fopen("maze", "w");
    if (!file)
    {
        perror("Failed to open file");
        free(maze);
        return 1;
    }

    fprintf(file, "type maze\n");
    fprintf(file, "height %d\n", height);
    fprintf(file, "width %d\n", width);
    fprintf(file, "map\n");

    for (int i = 0; i < height; i++)
    {
        fprintf(file, "%s\n", maze[i]);
    }

    fclose(file);
    for (int i = 0; i < height; i++)
    {
        free(maze[i]);
    }
    free(maze);

    printf("Maze generated successfully: maze.map (%dx%d)\n", width, height);

    return 0;
}
