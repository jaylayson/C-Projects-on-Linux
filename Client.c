#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>

#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

/* Supplied number of bytes */
#define BUFFERSIZE 200

/*
 * Game logic related definitions
 */
#define NUM_TILES_X 9
#define NUM_TILES_Y 9
#define NUM_MINES 10
int mines = NUM_MINES;
bool gameOver = false;
bool wonGame = false;
bool hitMine = false;
int convertedTime, convertedPlayed, convertedWins;
int timeTaken, gamesWon, gamesPlayed;

/*
 * Some variables for socket and buffer transfers
 */
int sockfd;
char buffer[BUFFERSIZE];
struct hostent *he;
struct sockaddr_in their_addr; /* CLIENT/CONNECTOR ADDRESS INFORMATION */


/*
 * Data structure for storing the Minesweeper grid.
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
 * Check command line arguments.
 */
void checkArg(int argc, char *argv[]){
	if (argc != 3) {
		fprintf(stderr,"usage: hostname portnumber. 2 Parameters.\n");
		exit(1);
	}

	if ((he=gethostbyname(argv[1])) == NULL) {  /* GET HOST INFO*/
		herror("gethostbyname");
		exit(1);
	}
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

	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(atoi(argv[2])); /* CONVERT INT ARGUMENT INTO NETWORK BYTE ORDER */
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	bzero(&(their_addr.sin_zero), 8);/* ZERO THE REST OF STRUCT */


}

/*
 * Initiate connection on socket.
 */
void initConnection(int sockfd, struct sockaddr_in their_addr){
	if (connect(sockfd, (struct sockaddr *)&their_addr, \
	sizeof(struct sockaddr)) == -1) {
		perror("connect");
		exit(1);
	}
}

/*
 * Print welcome page/banner.
 */
void welcomePage() {
	printf("\n=========================================================\n");
	printf("\n     Welcome to the Online Minesweeper Gaming System     \n");
	printf("\n=========================================================\n");
	printf("You are required to log on with your registered user name and password.\n");
}


/*
 * Prompt client for username and password.
 */
void login(char *u, char *p){
	printf("\nUsername: ");
	scanf(" %s", u);
	printf("\n");
	printf("Password: ");
	scanf(" %s", p);
}

/*
 * Check selection validity, 1 2 or 3 only.
 */
int selectionOutput() {
	char input[BUFFERSIZE];
	int check = 0;

	while (1) {
		scanf(" %s", input);
		//printf("option chosen %s", input);
		if (atoi(input) >= 1 && atoi(input) <= 3) {
			break;
		} else {
			printf("\n\nNo such option. Enter a valid selection( 1 2 or 3 ).\n");

		}
	}
	check = atoi(input);
	return check;
}

/*
* Prompt user for main menu selection.
 */
int mainMenu(){
	char chosen[BUFFERSIZE];
	int i;
	printf("\n\nWelcome to the Minesweeper Gaming System\n");
	printf("Please enter a selection:\n");
	printf("<1> Play Minesweeper\n");
	printf("<2> Show Leaderboard\n");
	printf("<3> Quit\n");

	switch(selectionOutput()){

		case 1:
			i = 1;
			strncpy(chosen, "PLAY", BUFFERSIZE);
			send(sockfd, chosen , sizeof(chosen), 0);
			return i;
		case 2:
      i = 2;
			strncpy(chosen, "LEADERBOARD", BUFFERSIZE);
			send(sockfd, chosen, sizeof(chosen), 0);
      return i;
		case 3:
      i = 3;
			strncpy(chosen, "QUIT", BUFFERSIZE);
			send(sockfd, chosen, sizeof(chosen), 0);
      return i;

	};
	return i;
};

/**
 * Print the minesweeper grid.
 * @param tile1 struct to be printed onto the screen.
 */
void printBoard(indivTile tile1[9][9]){
  printf("==================================================\n");
  printf("                    Minesweeper                   \n");
  printf("==================================================\n");
  printf("Remaining Mines: %d\n\n", mines);
  printf("      1 2 3 4 5 6 7 8 9 \n");
  printf("  ---------------------- ");

  char row = 'A';
  for (int i = 0; i < NUM_TILES_X; i++)
  {
    for (int j = 0; j < NUM_TILES_Y; j++)
    {
      if (j%9 == 0)
      {
        printf("\n");
        printf("  %c | ", row);
        row++;
      }
      if (tile1[i][j].revealed == false && tile1[i][j].flagged == false)
      {
        printf("  ");
      }
      else if (tile1[i][j].revealed == true)
      {
        if (tile1[i][j].is_mine == true)
        {
          printf("* ");
        }
        else
        {
          printf("%d ", tile1[i][j].adjacent_mines);
        }
      }
      else if (tile1[i][j].flagged == true)
      {
        printf("f ");
      }
    }
  }
};


int main(int argc, char *argv[]){
/* ESTABLISH A CONNECTION */
checkArg(argc, argv);

sockfd = createSocket(sockfd);

setSocketAttributes(argv);

initConnection(sockfd, their_addr);

welcomePage();
	char username [25];
	char *namePtr;
	namePtr = username;

	char password [25];
	char *passPtr;
	passPtr = password;

	char status[BUFFERSIZE]; //Value holder for received Authentication
	int n; //Value holder for mainMenu()
	char option[BUFFERSIZE]; //Reveal, Flag or Quit

	char coordRow[BUFFERSIZE];
	char coordCol[BUFFERSIZE];
  char gameResult[BUFFERSIZE];

	login(namePtr, passPtr);

  send(sockfd, username, sizeof(username), 0);
  send(sockfd, password, sizeof(password), 0);

if(recv(sockfd, &status, sizeof(status),0) > 0){
	printf("Authentication Received: %s\n", status);
}

//Disconnect if the incorrect credentials were entered.
if(strcmp(status, "You entered either an incorrect Username or Password. \n Disconnecting.") == 0){
	exit(1);
}

//Initialise board
indivTile tile1[9][9];

while(1){
  n = mainMenu();
  while(1){
  	//If user chose to play.
  	if(n == 1){
      //receive struct
  		recv(sockfd, &tile1, sizeof(tile1), 0);
      //receive mine count
      recv(sockfd, &mines, sizeof(mines), 0);

      printBoard(tile1);

  		printf("\n\nChoose an option:\n");
  		printf("<R> Reveal Tile\n");
  		printf("<P> Place Flag\n");
  		printf("<Q> Quit Game\n\n");
  		printf("Options (R, P, Q): ");
  		scanf(" %s", option); //Get input
  		send(sockfd, option, sizeof(option), 0); //Send R, P, Q

      //Reveal
  		if(strcmp(option, "r") == 0 || strcmp(option, "R") == 0){
  			printf("Enter a coordinate to reveal: ");
  			scanf(" %c%c", coordRow, coordCol);
  			send(sockfd, coordRow, sizeof(coordRow), 0);
  			send(sockfd, coordCol, sizeof(coordCol), 0);
        bzero(coordCol, BUFFERSIZE);
        bzero(coordRow, BUFFERSIZE);
        bzero(option, BUFFERSIZE);

  		}else if(strcmp(option, "p") == 0|| strcmp(option, "P") == 0){ //Place Flag
  			printf("Enter a coordinate to flag: ");
        scanf(" %c%c", coordRow, coordCol);
  			send(sockfd, coordRow, sizeof(coordRow), 0);
  			send(sockfd, coordCol, sizeof(coordCol), 0);
        bzero(coordCol, BUFFERSIZE);
        bzero(coordRow, BUFFERSIZE);

  		}else if(strcmp(option, "q") == 0|| strcmp(option, "Q") == 0){//Quit
        break;
  		}

      //Check gameResult status
      if(recv(sockfd, &gameResult, sizeof(gameResult), 0) > 0){
        //Continue if no mines are hit
        if(strcmp(gameResult, "CONTINUE") == 0){
          continue;
        }

        //Go back to menu after losing
        if(strcmp(gameResult, "GAMEOVER") == 0){
              recv(sockfd, &tile1, sizeof(tile1), 0);//new try update
              printBoard(tile1);
              printf("\nGame Over! You hit a mine.\n");
              break;
        }
        //Go back to menu after winning
        if(strcmp(gameResult, "GAMEWON") == 0){
              printf("\nCongratulations! You won the game! \n");
              break;
        }
      }
    }//end of if(n == 1)
    //If user chose to view Leaderboard.
    if(n == 2){
      printf("Fetching Leaderboard...\n");
      recv(sockfd, &buffer, sizeof(buffer), 0);
      recv(sockfd, &timeTaken, sizeof(timeTaken), 0);
      recv(sockfd, &gamesWon, sizeof(gamesWon), 0);
      recv(sockfd, &gamesPlayed, sizeof(gamesPlayed), 0);
      convertedTime = ntohs(timeTaken);
      convertedWins = ntohs(gamesWon);
      convertedPlayed = ntohs(gamesPlayed);
      printf("==================================================\n");
      printf("                  LEADERBOARD                      \n");
      printf("==================================================\n");
      printf(" Name          Time         Wins     Played\n");
      printf("%s            %ds           %d       %d\n", buffer, convertedTime,
                    convertedWins, convertedPlayed);
      break;
    }

    //If user chose to exit, close socket.
    if(n == 3){
      close(sockfd);
      exit(1);
    }
  }//end of Game while
}//end of While (menu)
  //close(sockfd);
    return 0;
}
