#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>

#define NO_ITEMS 100
int fd, n, m, buffer = 0;
int board_player1[10][10] = {0};
int shots_player1[10][10] = {0};
int board_player2[10][10] = {0};
int shots_player2[10][10] = {0};
int ships_number[2] = {6, 6};
int count = 3;
int cols = 10;
int rows = 10;

pthread_mutex_t mutex;
pthread_cond_t cond_player2_consumer, cond_player1_producer;

// Prototypes
void print_board(int matrix[10][10]);
void ships(int matrix[10][10], int p);
void wait_for_enter();
void check_for_hit(int player_q[10][10], int shots_q[10][10], int p, int q);
void signal_handler(int signum);
void alarm_handler(int signum);
void* player1_producer(void* arg);
void* player2_consumer(void* arg);
void menu();

int main()
{
    int fd, win, m;
    int status;
    pid_t son;
    pthread_t thread_1, thread_2;
    signal(SIGALRM, alarm_handler);
    son = fork();
    
    if (son == 0)
    {
        // Initializing player boards
        menu();
        printf("\nBOARD PLAYER 1\n");
        print_board(board_player1);
        ships(board_player1, 1);
        printf("\nBOARD PLAYER 2\n");
        print_board(board_player2);
        ships(board_player2, 2);
          
        signal(SIGUSR2, signal_handler);
        signal(SIGUSR1, signal_handler);
          
        pthread_mutex_init( &mutex, 0 );
        pthread_cond_init( &cond_player2_consumer, 0 );
        pthread_cond_init( &cond_player1_producer, 0 );
          
        // Create threads
        pthread_create( &thread_1, NULL, player1_producer, NULL );
        pthread_create( &thread_2, NULL, player2_consumer, NULL );
          
        // Join threads
        pthread_join( thread_1, NULL);
        pthread_join( thread_2, NULL);
        
        // Destroy mutex and conditionals
        pthread_mutex_destroy( &mutex );
        pthread_cond_destroy( &cond_player2_consumer );
        pthread_cond_destroy( &cond_player1_producer );
        return 0;
    }
    else if (son > 0) // Main process
    {
        // Waits for process "son" to end,
        //then reads the new document to print winner
        waitpid(son, &status, 0);
        printf("We have the results! The winner is...\n");
        // Opening envelope, building suspense...
        fd = open("winner.txt", 0);
        m = read (fd, &win, sizeof(win));
        alarm(3);
        pause();
        printf("PLAYER %d, YOU ARE THE WINNER!!\n", win);
        return 0;
    }
}

//Print board of player 'p'
void print_board(int matrix[10][10]) 
{
  // Printing index line from the top
  printf("    ");
  for (int i = 1; i <= rows; i++) 
  {
      printf("%3d ", i);
  }
  printf("\n");
  for (int i = 0; i < cols; i++) 
  {
      printf("%3d ", i+1); // Prints the index located on the left of the board
      for (int j = 0; j < cols; j++) 
      {
          // Prints a special character depending on the number in the board
          if (matrix[i][j] == 0) // Sea
          {
              printf("%3c ", '~');
          }
          else if (matrix[i][j] == 1) // Ship
          {
              printf("%3c ", 'S');
          }
          else if (matrix[i][j] == 2) // Missed shot
          {
              printf("%3c ", 'M');
          }
          else if (matrix[i][j] == 3) // Hit a ship/sunken
          {
              printf("%3c ", 'X');
          }
          else 
          {
              printf("%3d ", matrix[i][j]);
          }
      }
      printf("\n");
  }
}

// Place the 2x2 ships of the players
void ships(int matrix[10][10], int p)
{
  int x, y, orientation;
  printf("\nPLAYER %d\n", p);
  
  // Place a 2x2 ship on the player's board
  for (int i = 0; i < count; i++) 
  {
    printf("\nEnter the position of the ship #%d: ", i+1);
    scanf("%d %d", &x, &y);

    // Ask for orientation
    printf("Enter the orientation of the ship (0 for vertical, 1 for horizontal): ");
    scanf("%d", &orientation);

    // Checking if the ship can be placed vertically
    if (orientation == 0) 
    {
      if (y < 1 || y > cols - 1) 
      {
        printf("Error: Invalid column number.\n");
        i--;
        continue;
      }

      if (x < 1 || x > rows - 2) 
      {
        printf("Error: Invalid row number.\n");
        i--;
        continue;
      }

      // Already occupied space
      if (matrix[x-1][y-1] == 1 || matrix[x][y-1] == 1) 
      {
        printf("Error: That position is already occupied.\n");
        i--;
        continue;
      }

      // Place 2x2 ship on the player's board vertically
      matrix[x-1][y-1] = 1;
      matrix[x][y-1] = 1;
    } 
    // Checking if the ship can be placed horizontally
    else if (orientation == 1) 
    {
      if (y < 1 || y > cols - 2) 
      {
        printf("Error: Invalid column number.\n");
        i--;
        continue;
      }

      if (x < 1 || x > rows - 1) 
      {
        printf("Error: Invalid row number.\n");
        i--;
        continue;
      }

      // Already occupied space
      if (matrix[x-1][y-1] == 1 || matrix[x-1][y] == 1) 
      {
        printf("Error: That position is already occupied.\n");
        i--;
        continue;
      }

      // Place 2x2 ship on the player's board horizontally
      matrix[x-1][y-1] = 1;
      matrix[x-1][y] = 1;
    } 
    else 
    {
      printf("Error: Invalid orientation.\n");
      i--;
      continue;
    }
  }
}

// Waits to receive an enter in order to change turns
void wait_for_enter()
{
  printf("\nPress ENTER to coninue...");
  getchar(); // Consumes registered enter
}

// Check the attack from de player 'p'
void check_for_hit(int player_q[10][10], int shots_q[10][10], int p, int q)
{
  int x, y;
  printf("\nPLAYER %d", p);
  printf("\nEnter the position to attack: ");
  scanf("%d %d", &x, &y);
  getchar();
  if (player_q[x-1][y-1] == 1) // Player hit a ship
  {
    player_q[x-1][y-1] = 3;
    shots_q[x-1][y-1] = 3;
    sleep(1);
    ships_number[q-1]--;
    //ships_number2--;
    printf("Attack of player %d has hit a ship.\nAttack again!\n", p);
    sleep(2);
    check_for_hit(player_q, shots_q, p, q);
  }
  else // Player missed
  {
    shots_q[x-1][y-1] = 2;
    sleep(1);
    printf("\nAttack of player %d failed.\nNext turn!\n", p);
  }
  
  if(ships_number[q-1] == 0 ) // The other player has no ships left
  {
    pthread_kill(pthread_self(), SIGUSR1);
  }
}

// Signal handler to print the message of the player that wins the game
void signal_handler(int signum)
{
  if (signum == SIGUSR1) 
  {
        int winner = 1;
        fd = creat("winner.txt", 777);
        n = write(fd, &winner, sizeof(winner));
        close(fd);
        exit(0);
  }
  else if (signum == SIGUSR2)
  {
        int winner = 2;
        fd = creat("winner.txt", 777);
        n = write(fd, &winner, sizeof(winner));
        close(fd);
        exit(0);
  }
}

// Player 1 has the role of the producer
void* player1_producer(void* arg)
{
  for ( int i=0; i<NO_ITEMS; i++ ) 
  {
    pthread_mutex_lock(&mutex);
    while( buffer != 0 ) // Wait for buffer to be free
    {
      // Release mutex and waits for condition to be met
      pthread_cond_wait( &cond_player1_producer, &mutex );
    }
    buffer = 1;
    printf("\n\n");
    printf("BOARD PLAYER 2\n");
    print_board(shots_player2);
    printf("\nBOARD PLAYER 1\n");
    print_board(board_player1);
    check_for_hit(board_player2, shots_player2, 1, 2);
    wait_for_enter();
    //system("clear");
    
    // There is elements in the buffer
    pthread_cond_signal( &cond_player2_consumer );
    pthread_mutex_unlock( &mutex );
  }

  // Send signal when producer finishes
  pthread_kill( pthread_self(), SIGUSR1 );
  pthread_exit( NULL );
}

//Player 2 has the role of the consumer
void* player2_consumer(void* arg)
{
  for ( int i=0; i<NO_ITEMS; i++ ) 
  {
    pthread_mutex_lock( &mutex );
    // ---------- Critical region
    while (buffer == 0) // Waits for buffer to have data
    {
      // Release mutex and waits for condition to be met
      pthread_cond_wait( &cond_player2_consumer, &mutex );
    }
    printf("\n\n");
    printf("\n\n");
    printf("BOARD PLAYER 1\n");
    print_board(shots_player1);
    printf("\nBOARD PLAYER 2\n");
    print_board(board_player2);
    check_for_hit(board_player1, shots_player1, 2, 1);
    wait_for_enter();
    //system("clear");
  
    buffer = 0;
    
    // Lets know that buffer is clean
    pthread_cond_signal( &cond_player1_producer ); 
    pthread_mutex_unlock( &mutex );
  }

  // Sends signal when consumer is done
  pthread_kill( pthread_self(), SIGUSR2 );
  pthread_exit(NULL);
}

// Waits for hit or miss confirmation of a turn
void alarm_handler(int signum) {}

void menu()
{
    printf("Welcome to BattleShip!\n");
    printf("----Instrucions:----\n");
    printf("Both player must first place 3 boats of sixe 2x1, and they will take turns in order to try to guess\n");
    printf("the location of the other player's ships and sink them. The first player to sink all of the other\n");
    printf("player's ships WINS.\n");
    printf("S = ship\n");
    printf("M = Missed hit\n");
    printf("X = downed ship\n");
    printf("~ = water\nPress 'Enter' when both players are ready\n");
    wait_for_enter();
}
