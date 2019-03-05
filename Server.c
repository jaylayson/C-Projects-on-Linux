#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

/*
 * Socket related definitions
 */
#define CONNECTIONS 20
#define BUFFER_SIZE 200
#define PORTNUMBER 12345

/*
 * Game logic related definitions
 */
#define NUM_TILES_X 9
#define NUM_TILES_Y 9
#define NUM_MINES 10
#define RANDOM_NUMBER_SEED 42

/*
 * Some variables for socket and buffer transfers.
 */
int sockfd, newfd, portNo;
struct sockaddr_in server_addr, their_addr;
socklen_t sin_size; /* HOLD SOCKET SIZE */
char buffer[BUFFER_SIZE];

/*
 * Data structure for storing names and passwords
 * from the Authentication file.
 */
typedef struct registeredUsers {
	char *username;
	char *password;
	struct registeredUsers *next;
} User;
User *first = NULL;
User *last = NULL;
User *node;

/*
 * Data structure for storing the Minesweeper
 * grid.
 */
#pragma pack(1)
typedef struct Tile
{
  int adjacent_mines;
  bool revealed;
  bool is_mine;
  bool flagged;
  int tile;
} indivTile;
#pragma pack(0)


/*
 * Game related variables.
 */
int mines = NUM_MINES;
bool gameWon = false;
bool hitMine = false;
bool startedTimer = false;
bool quit = false;
int start, end, timeTaken, gamesWon = 0, gamesPlayed = 0;
int convertedInt;
int convertedGamesWonInt;
int convertedGamesPlayedInt;

/*
 * Check command line arguments.
 */
int checkArg(int argc, char *argv[]){
	if (argc != 2) {
		fprintf(stderr,"usage: client_port_number. 1 parameter only.\n");
		exit(1);
	}else if(argc == 2) {
		portNo = atoi(argv[1]);

	}else{
		portNo = 12345;
		printf("\nThe Server will listen on port: %d\n", portNo);
  }
  return portNo;
}
/*
 * Creates socket.
 */
int createSocket(int sockfd){
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	return sockfd;
}


/*
 * Specifies the attributes/address structure of the socket.
 */
void setSocketAttributes(char *argv[]){
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[1])); /* CONVERT INT ARGUMENT INTO NETWORK BYTE ORDER */
	server_addr.sin_addr.s_addr = INADDR_ANY; /* AUTOFILL WITH MY ADDRESS */
}

/*
 * Socket binding.
 */
void bindSocket(int sockfd, struct sockaddr_in server_addr){
	if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
		perror("bind");
		exit(1);
	}
}

/*
 * Listen through socket.
 */
void startListening(int sockfd){
	if (listen(sockfd, CONNECTIONS) == -1) {
		perror("listen");
		exit(1);
	}
	printf("Server: Starts listening ...\n");
}

/*
 * Read authentication file.
 */
void parseAuthentication(){
	char line[BUFFER_SIZE];
	char susername[BUFFER_SIZE];
	char spassword[BUFFER_SIZE];
	bool skip = true;


	FILE *fp;
	fp = fopen("Authentication.txt", "r");

	if(fp == NULL){
		printf("Can't open Authentication.txt\n");
		exit(1);
	}

	/* READ UNTIL THE END OF FILE */
	while(fgets(line, sizeof(line), fp) != NULL){

 		if(skip){ //Skip the first line entry, (Username, Password)
			skip = false;
			continue;
			}

		if (line[0] == '\0') {
			printf ("Line too short\n");
			exit(1);
		}

        	line[strlen (line)-1] = '\0';

		// Scan the individual fields.
		if (sscanf (line, "%s %s", susername, spassword) != 2){
				printf ("Line '%s' didn't scan properly\n", line);
				exit(1);
		}
		node = malloc (sizeof (User));
		if (node == NULL) {
			printf ("Ran out of memory\n");
			exit(1);
		}

		node->username = strdup(susername);
        	node->password = strdup(spassword);
		node->next = NULL;
		if (first != NULL) {
		    last->next = node;
		    last = node;
		} else {
		    first = node;
		    last = node;
		}
	}

	fclose(fp);

}

/**
 * Takes user input and matches with the Data
 * parsed from authentication.txt
 * @param  recUser [received username input from client]
 * @param  recPass [received username input from client]
 * @return         [true or false appropriately on string match result]
 */
bool authenticate(char *recUser, char *recPass){

	bool authenticated = false;
	node = first;
	while(node != NULL) {
		if(strcmp(recUser, node->username) == 0 && atoi(recPass) == atoi(node->password)){
			printf("User: %s \n", node->username);
			printf("Password: %d\n", atoi(node->password));
			authenticated = true;
				};

		node = node->next;
		}
  return authenticated;
}

/*
 * Receives the selection input from client.
 * x holds and returns appropriate value of a selection.
 */
int menu(){
	int x;
	char selection[BUFFER_SIZE];

	recv(newfd, &selection, sizeof(selection), 0);

	if(strcmp(selection, "PLAY") == 0){
		x = 1;
	} else if (strcmp(selection, "LEADERBOARD") == 0){
		x = 2;
		} else if(strcmp(selection, "QUIT") == 0){
			x = 3;
			};
	return x;
}

/**
 * [tile_contains_mine description]
 * @param  [name] [description]
 * @param  x      [description]
 * @param  y      [description]
 * @return        [description]
 */
bool tile_contains_mine(indivTile tile1[9][9], int x, int y)
{
  if (tile1[x][y].is_mine == true)
  {
    return true;
  }
  else
  {
    tile1[x][y].is_mine = true;
    return false;
  }
}


/**
 * [place_mines description]
 * @param [name] [description]
 */
void place_mines(indivTile tile1[9][9])
{
  for(int i = 0; i < NUM_MINES; i++)
  {
    int x, y;
    do
    {
      x = rand() % NUM_TILES_X;
      y = rand() % NUM_TILES_Y;
    } while (tile_contains_mine(tile1, x, y));
    // place mine at (x, y)
  }
}


/**
 * [setup_board description]
 * @param [name] [description]
 */
void setup_board(indivTile tile1[9][9])
{
  for (int i = 0; i < NUM_TILES_X; i++)
  {
    for (int j = 0; j < NUM_TILES_Y; j++)
    {
      tile1[i][j].revealed = false;
      tile1[i][j].is_mine = false;
      tile1[i][j].flagged = false;
    }
  }
  mines = NUM_MINES;
  place_mines(tile1);
}

/**
 * [check_surrounding_mines description]
 * @param  [name] [description]
 * @param  r      [description]
 * @param  c      [description]
 * @return        [description]
 */
int check_surrounding_mines(indivTile tile1[9][9], int r, int c)
{
  int topLeft = 0, top = 0, topRight = 0, right = 0,
  bottomRight = 0, bottom = 0, bottomLeft = 0, left = 0;
  int sum = 0;

  if (r == 0)
  {
    if (c == 0)
    {

      if (tile1[r][c+1].is_mine == true)
      {
        right = 1;

      }
      if (tile1[r+1][c].is_mine == true)
      {
        bottom = 1;
      }
      if (tile1[r+1][c+1].is_mine == true)
      {
        bottomRight = 1;
      }
    }
    else if (c == 8)
    {
      if (tile1[r][c-1].is_mine == true)
      {
        left = 1;
      }
      if (tile1[r+1][c-1].is_mine == true)
      {
        bottomLeft = 1;
      }
      if (tile1[r+1][c].is_mine == true)
      {
        bottom = 1;
      }
    }
    else
    {
      if (tile1[r][c-1].is_mine == true)
      {
        left = 1;
      }
      if (tile1[r+1][c-1].is_mine == true)
      {
        bottomLeft = 1;
      }
      if (tile1[r+1][c].is_mine == true)
      {
        bottom = 1;
      }
      if (tile1[r+1][c+1].is_mine == true)
      {
        bottomRight = 1;
      }
      if (tile1[r][c+1].is_mine == true)
      {
        right = 1;
      }
    }
  }
  else if (r == 8)
  {
    if (c == 0)
    {
      if (tile1[r-1][c].is_mine == true)
      {
        top = 1;
      }
      if (tile1[r-1][c+1].is_mine == true)
      {
        topRight = 1;
      }
      if (tile1[r][c+1].is_mine == true)
      {
        right = 1;
      }
    }
    else if (c == 8)
    {
      if (tile1[r][c-1].is_mine == true)
      {
        left = 1;
      }
      if (tile1[r-1][c-1].is_mine == true)
      {
        topLeft = 1;
      }
      if (tile1[r-1][c].is_mine == true)
      {
        top = 1;
      }
    }
    else
    {
      if (tile1[r][c-1].is_mine == true)
      {
        left = 1;
      }
      if (tile1[r-1][c-1].is_mine == true)
      {
        topLeft = 1;
      }
      if (tile1[r-1][c].is_mine == true)
      {
        top = 1;
      }
      if (tile1[r-1][c+1].is_mine == true)
      {
        topRight = 1;
      }
      if (tile1[r][c+1].is_mine == true)
      {
        right = 1;
      }
    }
  }
  else
  {
    if (c == 0)
    {
      if (tile1[r-1][c].is_mine == true)
      {
        top = 1;
      }
      if (tile1[r-1][c+1].is_mine == true)
      {
        topRight = 1;
      }
      if (tile1[r][c+1].is_mine == true)
      {
        right = 1;
      }
      if (tile1[r+1][c+1].is_mine == true)
      {
        bottomRight = 1;
      }
      if (tile1[r+1][c].is_mine == true)
      {
        bottom = 1;
      }
    }
    else if (c == 8)
    {
      if (tile1[r-1][c].is_mine == true)
      {
        top = 1;
      }
      if (tile1[r-1][c-1].is_mine == true)
      {
        topLeft = 1;
      }
      if (tile1[r][c-1].is_mine == true)
      {
        left = 1;
      }
      if (tile1[r+1][c-1].is_mine == true)
      {
        bottomLeft = 1;
      }
      if (tile1[r+1][c].is_mine == true)
      {
        bottom = 1;
      }
    }
    else
    {
      if (tile1[r-1][c-1].is_mine == true)
      {
        topLeft = 1;
      }
      if (tile1[r-1][c].is_mine == true)
      {
        top = 1;
      }
      if (tile1[r-1][c+1].is_mine == true)
      {
        topRight = 1;
      }
      if (tile1[r][c+1].is_mine == true)
      {
        right = 1;
      }
      if (tile1[r+1][c+1].is_mine == true)
      {
        bottomRight = 1;
      }
      if (tile1[r+1][c].is_mine == true)
      {
        bottom = 1;
      }
      if (tile1[r+1][c-1].is_mine == true)
      {
        bottomLeft = 1;
      }
      if (tile1[r][c-1].is_mine == true)
      {
        left = 1;
      }
    }
  }
  sum = topLeft + top + topRight + right + bottomRight + bottom + bottomLeft + left;
  return sum;
}

/**
 * [check_surrounding_tiles description]
 * @param [name] [description]
 * @param r      [description]
 * @param c      [description]
 */
void check_surrounding_tiles(indivTile tile1[9][9], int r, int c)
{
  if (tile1[r][c].adjacent_mines == 0)
  {
    if (r == 0)
    {
      if (c == 0)
      {

        if (tile1[r][c+1].adjacent_mines == 0 && tile1[r][c+1].revealed == false)
        {
          tile1[r][c+1].revealed = true;
					check_surrounding_tiles(tile1, r, c+1);
        }
        if (tile1[r+1][c].adjacent_mines == 0 && tile1[r+1][c].revealed == false)
        {
          tile1[r+1][c].revealed = true;
					check_surrounding_tiles(tile1, r+1, c);
        }
        if (tile1[r+1][c+1].adjacent_mines == 0 && tile1[r+1][c+1].revealed == false)
        {
          tile1[r+1][c+1].revealed = true;
					check_surrounding_tiles(tile1, r+1, c+1);
        }
      }
      else if (c == 8)
      {
        if (tile1[r][c-1].adjacent_mines == 0 && tile1[r][c-1].revealed == false)
        {
          tile1[r][c-1].revealed = true;
					check_surrounding_tiles(tile1, r, c-1);
        }
        if (tile1[r+1][c-1].adjacent_mines == 0 && tile1[r+1][c-1].revealed == false)
        {
          tile1[r+1][c-1].revealed = true;
					check_surrounding_tiles(tile1, r+1, c-1);
        }
        if (tile1[r+1][c].adjacent_mines == 0 && tile1[r+1][c].revealed == false)
        {
          tile1[r+1][c].revealed = true;
					check_surrounding_tiles(tile1, r+1, c);
        }
      }
      else
      {
        if (tile1[r][c-1].adjacent_mines == 0 && tile1[r][c-1].revealed == false)
        {
          tile1[r][c-1].revealed = true;
					check_surrounding_tiles(tile1, r, c-1);
        }
        if (tile1[r+1][c-1].adjacent_mines == 0 && tile1[r+1][c-1].revealed == false)
        {
          tile1[r+1][c-1].revealed = true;
					check_surrounding_tiles(tile1, r+1, c-1);
        }
        if (tile1[r+1][c].adjacent_mines == 0 && tile1[r+1][c].revealed == false)
        {
          tile1[r+1][c].revealed = true;
					check_surrounding_tiles(tile1, r, c);
        }
        if (tile1[r+1][c+1].adjacent_mines == 0 && tile1[r+1][c+1].revealed == false)
        {
          tile1[r+1][c+1].revealed = true;
					check_surrounding_tiles(tile1, r+1, c+1);
        }
        if (tile1[r][c+1].adjacent_mines == 0 && tile1[r][c+1].revealed == false)
        {
          tile1[r][c+1].revealed = true;
					check_surrounding_tiles(tile1, r, c+1);
        }
      }
    }
    else if (r == 8)
    {
      if (c == 0)
      {
        if (tile1[r-1][c].adjacent_mines == 0 && tile1[r-1][c].revealed == false)
        {
          tile1[r-1][c].revealed = true;
					check_surrounding_tiles(tile1, r-1, c);
        }
        if (tile1[r-1][c+1].adjacent_mines == 0 && tile1[r-1][c+1].revealed == false)
        {
          tile1[r-1][c+1].revealed = true;
					check_surrounding_tiles(tile1, r-1, c+1);
        }
        if (tile1[r][c+1].adjacent_mines == 0 && tile1[r][c+1].revealed == false)
        {
          tile1[r][c+1].revealed = true;
					check_surrounding_tiles(tile1, r, c+1);
        }
      }
      else if (c == 8)
      {
        if (tile1[r][c-1].adjacent_mines == 0 && tile1[r][c-1].revealed == false)
        {
          tile1[r][c-1].revealed = true;
					check_surrounding_tiles(tile1, r, c-1);
        }
        if (tile1[r-1][c-1].adjacent_mines == 0 && tile1[r-1][c-1].revealed == false)
        {
          tile1[r-1][c-1].revealed = true;
					check_surrounding_tiles(tile1, r-1, c-1);
        }
        if (tile1[r-1][c].adjacent_mines == 0 && tile1[r-1][c].revealed == false)
        {
          tile1[r-1][c].revealed = true;
					check_surrounding_tiles(tile1, r-1, c);
        }
      }
      else
      {
        if (tile1[r][c-1].adjacent_mines == 0 && tile1[r][c-1].revealed == false)
        {
          tile1[r][c-1].revealed = true;
					check_surrounding_tiles(tile1, r, c-1);
        }
        if (tile1[r-1][c-1].adjacent_mines == 0 && tile1[r-1][c-1].revealed == false)
        {
          tile1[r-1][c-1].revealed = true;
					check_surrounding_tiles(tile1, r-1, c-1);
        }
        if (tile1[r-1][c].adjacent_mines == 0 && tile1[r-1][c].revealed == false)
        {
          tile1[r-1][c].revealed = true;
					check_surrounding_tiles(tile1, r-1, c);
        }
        if (tile1[r-1][c+1].adjacent_mines == 0 && tile1[r-1][c+1].revealed == false)
        {
          tile1[r-1][c+1].revealed = true;
					check_surrounding_tiles(tile1, r, c);
        }
        if (tile1[r][c+1].adjacent_mines == 0 && tile1[r][c+1].revealed == false)
        {
          tile1[r][c+1].revealed = true;
					check_surrounding_tiles(tile1, r, c+1);
        }
      }
    }
    else
    {
      if (c == 0)
      {
        if (tile1[r-1][c].adjacent_mines == 0 && tile1[r-1][c].revealed == false)
        {
          tile1[r-1][c].revealed = true;
					check_surrounding_tiles(tile1, r-1, c);
        }
        if (tile1[r-1][c+1].adjacent_mines == 0 && tile1[r-1][c+1].revealed == false)
        {
          tile1[r-1][c+1].revealed = true;
					check_surrounding_tiles(tile1, r-1, c+1);
        }
        if (tile1[r][c+1].adjacent_mines == 0 && tile1[r][c+1].revealed == false)
        {
          tile1[r][c+1].revealed = true;
					check_surrounding_tiles(tile1, r, c+1);
        }
        if (tile1[r+1][c+1].adjacent_mines == 0 && tile1[r+1][c+1].revealed == false)
        {
          tile1[r+1][c+1].revealed = true;
					check_surrounding_tiles(tile1, r+1, c+1);
        }
        if (tile1[r+1][c].adjacent_mines == 0 && tile1[r+1][c].revealed == false)
        {
          tile1[r+1][c].revealed = true;
					check_surrounding_tiles(tile1, r+1, c);
        }
      }
      else if (c == 8)
      {
        if (tile1[r-1][c].adjacent_mines == 0 && tile1[r-1][c].revealed == false)
        {
          tile1[r-1][c].revealed = true;
					check_surrounding_tiles(tile1, r-1, c);
        }
        if (tile1[r-1][c-1].adjacent_mines == 0 && tile1[r-1][c-1].revealed == false)
        {
          tile1[r-1][c-1].revealed = true;
					check_surrounding_tiles(tile1, r-1, c-1);
        }
        if (tile1[r][c-1].adjacent_mines == 0 && tile1[r][c-1].revealed == false)
        {
          tile1[r][c-1].revealed = true;
					check_surrounding_tiles(tile1, r, c-1);
        }
        if (tile1[r+1][c-1].adjacent_mines == 0 && tile1[r+1][c-1].revealed == false)
        {
          tile1[r+1][c-1].revealed = true;
					check_surrounding_tiles(tile1, r+1, c-1);
        }
        if (tile1[r+1][c].adjacent_mines == 0 && tile1[r+1][c].revealed == false)
        {
          tile1[r+1][c].revealed = true;
					check_surrounding_tiles(tile1, r+1, c);
        }
      }
      else
      {
        if (tile1[r-1][c-1].adjacent_mines == 0 && tile1[r-1][c-1].revealed == false)
        {
          tile1[r-1][c-1].revealed = true;
					check_surrounding_tiles(tile1, r-1, c-1);
        }
        if (tile1[r-1][c].adjacent_mines == 0 && tile1[r-1][c].revealed == false)
        {
          tile1[r-1][c].revealed = true;
					check_surrounding_tiles(tile1, r-1, c);
        }
        if (tile1[r-1][c+1].adjacent_mines == 0 && tile1[r-1][c+1].revealed == false)
        {
          tile1[r-1][c+1].revealed = true;
					check_surrounding_tiles(tile1, r-1, c+1);
        }
        if (tile1[r][c+1].adjacent_mines == 0 && tile1[r][c+1].revealed == false)
        {
          tile1[r][c+1].revealed = true;
					check_surrounding_tiles(tile1, r, c+1);
        }
        if (tile1[r+1][c+1].adjacent_mines == 0 && tile1[r+1][c+1].revealed == false)
        {
          tile1[r+1][c+1].revealed = true;
					check_surrounding_tiles(tile1, r+1, c+1);
        }
        if (tile1[r+1][c].adjacent_mines == 0 && tile1[r+1][c].revealed == false)
        {
          tile1[r+1][c].revealed = true;
					check_surrounding_tiles(tile1, r+1, c);
        }
        if (tile1[r+1][c-1].adjacent_mines == 0 && tile1[r+1][c-1].revealed == false)
        {
          tile1[r+1][c-1].revealed = true;
					check_surrounding_tiles(tile1, r+1, c-1);
        }
        if (tile1[r][c-1].adjacent_mines == 0 && tile1[r][c-1].revealed == false)
        {
          tile1[r][c-1].revealed = true;
					check_surrounding_tiles(tile1, r, c-1);
        }
      }
    }
  }
}


/**
 * [set_tile_values description]
 * @param [name] [description]
 */
void set_tile_values(indivTile tile1[9][9])
{
  for (int i = 0; i < NUM_TILES_X; i++)
  {
    for (int j = 0; j < NUM_TILES_Y; j++)
    {
      tile1[i][j].adjacent_mines = check_surrounding_mines(tile1, i, j);
    }
  }
}


/**
 * [revealTile description]
 * @param [name] [description]
 */
void revealTile(indivTile tile1[9][9]){
	char rcvdRow[BUFFER_SIZE];
	char rcvdCol[BUFFER_SIZE];
	int row;
	int col;

	//Receive the coordinates
	recv(newfd, &rcvdRow, sizeof(rcvdRow), 0);
	recv(newfd, &rcvdCol, sizeof(rcvdCol), 0);

	col = atoi(rcvdCol) - 1;

	if (strcmp(rcvdRow, "a") == 0 || strcmp(rcvdRow, "A") == 0)
	{
		row = 0;
	}
	else if (strcmp(rcvdRow, "b") == 0 || strcmp(rcvdRow, "B") == 0)
	{
		row = 1;
	}
	else if (strcmp(rcvdRow, "c") == 0 || strcmp(rcvdRow, "C") == 0)
	{
		row = 2;
	}
	else if (strcmp(rcvdRow, "d") == 0 || strcmp(rcvdRow, "D") == 0)
	{
		row = 3;
	}
	else if (strcmp(rcvdRow, "e")  == 0 || strcmp(rcvdRow, "E")== 0 )
	{
		row = 4;
	}
	else if (strcmp(rcvdRow, "f") == 0 || strcmp(rcvdRow, "F")== 0 )
	{
		row = 5;
	}
	else if (strcmp(rcvdRow, "g") == 0 || strcmp(rcvdRow, "G")== 0 )
	{
		row = 6;
	}
	else if (strcmp(rcvdRow, "h") == 0 || strcmp(rcvdRow, "H")== 0 )
	{
		row = 7;
	}
	else if (strcmp(rcvdRow, "i") == 0 || strcmp(rcvdRow, "I")== 0 )
	{
		row = 8;
	}

	tile1[row][col].revealed = true; //Revealing

  if (tile1[row][col].revealed == true) //Is it revealed
  {
    if (tile1[row][col].is_mine == true)//Is it a mine
    {
      for (int i = 0; i < NUM_TILES_X; i++)
      {
        for (int j = 0; j < NUM_TILES_Y; j++)//Look through array
        {
          if (tile1[i][j].is_mine == true)//Checking for all mines
          {
            tile1[i][j].revealed = true;//Reveal all mines
            hitMine = true; //Hit a mine - True
          }
        }
      }
    }
    else
    {
			set_tile_values(tile1);
      check_surrounding_tiles(tile1, row, col);
    }
  }

}


/**
 * [placeFlag description]
 * @param [name] [description]
 */
void placeFlag(indivTile tile1[9][9])
{
	char rcvdRow[BUFFER_SIZE];
	char rcvdCol[BUFFER_SIZE];
	int row;
	int col;

	//Receive the coordinates
	recv(newfd, &rcvdRow, sizeof(rcvdRow), 0);
	recv(newfd, &rcvdCol, sizeof(rcvdCol), 0);

	col = atoi(rcvdCol) - 1;

	if (strcmp(rcvdRow, "a") == 0 || strcmp(rcvdRow, "A") == 0)
	{
		row = 0;
	}
	else if (strcmp(rcvdRow, "b") == 0 || strcmp(rcvdRow, "B") == 0)
	{
		row = 1;
	}
	else if (strcmp(rcvdRow, "c") == 0 || strcmp(rcvdRow, "C") == 0)
	{
		row = 2;
	}
	else if (strcmp(rcvdRow, "d") == 0 || strcmp(rcvdRow, "D") == 0)
	{
		row = 3;
	}
	else if (strcmp(rcvdRow, "e")  == 0 || strcmp(rcvdRow, "E")== 0 )
	{
		row = 4;
	}
	else if (strcmp(rcvdRow, "f") == 0 || strcmp(rcvdRow, "F")== 0 )
	{
		row = 5;
	}
	else if (strcmp(rcvdRow, "g") == 0 || strcmp(rcvdRow, "G")== 0 )
	{
		row = 6;
	}
	else if (strcmp(rcvdRow, "h") == 0 || strcmp(rcvdRow, "H")== 0 )
	{
		row = 7;
	}
	else if (strcmp(rcvdRow, "i") == 0 || strcmp(rcvdRow, "I")== 0 )
	{
		row = 8;
	}

  tile1[row][col].flagged = true;//Set tile to flag
  if (tile1[row][col].is_mine == true)
  {
    mines = mines - 1;
    if (mines == 0)
    {
      gameWon = true;
    }
  }
}


/**
 * Takes the user input R P or Q and calls Reveal
 * Place flag, or Quit appropriately.
 * @param tile1 tile1 is the placeholder for
 * 							the initialized struct in main
 */
void gameControl(indivTile tile1[9][9]){
	int i = 0;
	char option[BUFFER_SIZE];
	recv(newfd, &option, sizeof(option), 0);

	if(strcmp(option, "r") == 0 || strcmp(option, "R") == 0){
		printf("\n\nServer: Option %s\n", option);
		revealTile(tile1);

	}else if(strcmp(option, "p") == 0 || strcmp(option, "P") ==0){
		printf("\nServer: Option %s\n", option);
		placeFlag(tile1);

	}else if(strcmp(option, "q") == 0|| strcmp(option, "Q") == 0){
		printf("\nServer: Option %s\n", option);
		printf("QUIT\n");
		quit = true;
	}

}

void signalHandler(int signum){
	//signal(signum, SIG_IGN);
	printf("\n\n!SIGINT DETECTED!\n\n");
	printf("Exiting..\n");
	sleep(1);
	close(newfd);
	exit(1);
}


int main(int argc, char *argv[]){
	//Some buffer holders for connection transfer.
	char receivedUserName [25];
	char receivedPassword [25];
	char *rcvUserPtr;
	char *rcvPassPtr;
	rcvUserPtr = receivedUserName;
	rcvPassPtr = receivedPassword;
	char authResult[BUFFER_SIZE];
	char gameWonBuffer[BUFFER_SIZE];
	char gameOverBuffer[BUFFER_SIZE];
	char continuePlayBuffer[BUFFER_SIZE];
	bool authentication;

/* ESTABLISH A CONNECTION */
checkArg(argc, argv);
	printf("Server: Port Number is %d\n", portNo);//debug

sockfd = createSocket(sockfd);
	printf("Server: Socket created.\n");//debug

setSocketAttributes(argv);
bindSocket(sockfd, server_addr);
startListening(sockfd);
parseAuthentication();

signal(SIGINT,signalHandler);
while((newfd = accept(sockfd, (struct sockaddr *)&their_addr, \
		&sin_size))){
	int m, n; //Holds the value of recv. Used to check if the recv
						//call is successful
	sin_size = sizeof(struct sockaddr_in);

		printf("Server: got connection from %s\n", \
			inet_ntoa(their_addr.sin_addr));

	m = recv(newfd, &receivedUserName, sizeof(receivedUserName), 0);
		if (m < 0){
			perror("receive");
			exit(1);
			}else if(m > 0){
				printf("Server: Username received %s\n", receivedUserName);
				}
	n = recv(newfd, &receivedPassword, sizeof(receivedPassword), 0);
		if (n < 0){
			perror("receive");
			exit(1);
		}else if(n > 0){
				printf("Server: Password received %s\n", receivedPassword);
				}

	authentication = authenticate(rcvUserPtr, rcvPassPtr); //Authenticate the input

	if(authentication){
		printf("Server: %s logged in\n", receivedUserName);
		strncpy(authResult, "Successful login", BUFFER_SIZE);
		send(newfd, authResult, sizeof(authResult), 0);

	}else if(!authentication){
		strncpy(authResult, "You entered either an incorrect Username or Password. \n Disconnecting.", BUFFER_SIZE);
		send(newfd, authResult, sizeof(authResult), 0);
		continue;
	};


//START OF GAME LOGIC HERE
srand(RANDOM_NUMBER_SEED);

indivTile tile1[9][9];

/*
* While loop for 'Play again' logic.
 */
while(1){
mines = NUM_MINES; //Reset number of mines

/*
 * Reset bool values and variables
 */
gameWon = false;
hitMine = false;
startedTimer = false;
quit = false;
gamesPlayed = 0;
gamesWon = 0;
timetaken = 0;
int mm; //Holds menu output.
mm = menu();

setup_board(tile1);

	while(1){

		/*
		 * If user chose PLAY. 1 == PLAY.
		 */
		if(mm == 1){
			/*
			 * If timer has not yet started.
			 */
			if(startedTimer == false){
				start = time(NULL);
				startedTimer = true;
				gamesPlayed++;
			}

			send(newfd, &tile1, sizeof(tile1), 0);//Send the struct
			send(newfd, &mines, sizeof(mines), 0);//Send mine count

			gameControl(tile1);

			// If user chose to quit midway, go back to menu.
			if(quit == true){
				break;
			}
			//If no mines are hit and game isn't won(all mines are not flagged):
			//continue playing
				if(hitMine == false && gameWon == false){
					printf("Server: No mine hit\n");
					strncpy(continuePlayBuffer, "CONTINUE", BUFFER_SIZE);
					printf("%s\n", continuePlayBuffer);
					send(newfd, continuePlayBuffer, sizeof(continuePlayBuffer), 0);
				}

				//If a mine is revealed
				if(hitMine == true){
					printf("Server: Hit a mine\n");
					strncpy(gameOverBuffer, "GAMEOVER", BUFFER_SIZE);
					printf("%s\n", gameOverBuffer);

					end = time(NULL);
					timeTaken = end - start;

					send(newfd, gameOverBuffer, sizeof(gameOverBuffer), 0);
					send(newfd, &tile1, sizeof(tile1), 0);//Send the struct

					break; //Return to Menu
				}

				//If all mines are flagged
				if(gameWon == true){
					gamesWon++;
					end = time(NULL);
					timeTaken = end - start;
					printf("Flagged all mines\n");
					strncpy(gameWonBuffer, "GAMEWON", BUFFER_SIZE);
					printf("%s\n", gameWonBuffer);
					send(newfd, gameWonBuffer, sizeof(gameWonBuffer), 0);

					break; //Return to Menu
				}
		}
		/*
		 * If user chose LEADEARBOARD. 2 == LEADERBOARD
		 */
		if(mm == 2){
			//Convert the integers to network bytes.
			convertedInt = htons(timeTaken);
			convertedGamesWonInt = htons(gamesWon);
			convertedGamesPlayedInt = htons(gamesPlayed);

			//Send player data.
			send(newfd, receivedUserName, sizeof(receivedUserName), 0);
			sleep(1);
			send(newfd, &convertedInt, sizeof(convertedInt), 0);
			sleep(1);
			send(newfd, &convertedGamesWonInt, sizeof(convertedGamesWonInt), 0);
			sleep(1);
			send(newfd, &convertedGamesPlayedInt, sizeof(convertedGamesPlayedInt), 0);
			sleep(1);
			break; //Return to menu
		}
		/*
		 * If user chose QUIT. 3 == QUIT.
		 */
		if(mm == 3){
			break;
		}
	}//End of inner game loop
	if(mm == 3){
		break;//Return to listening
		}
	}//End of outer game loop

}//End of Main while




	close(newfd);
}
