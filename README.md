# Obj_C_systems_programming_projs

## Project 1 - Doom Train (Multithreaded Processes, network programming)
This project involves creating an abstract simulation of a transportation network. It will use pthreads, tcp networking and thread-safety. 
### Simulation

The simulation will consist of a number of “stations”.Each station may be connected to a number of other stationsvia network connections..Network messages representing “trains” will arrive via these network connections, pickup or deposit resouces and move on to the next station.

### Implementation
The single program written is called station.There  are  no  separate  client  and  server  programs.  Each station will listen on a single TCP pîrt for connections.  All stations in a simulation willstart  off  isolated  and  listening  on  their  designated  port.   Programs  connecting  to  this  port  will  supply  anauthentication string (one line of text) followed by their station name and a sequence of trains.  The stationname will be used to forward trains to that station if required.  The program makes use of multi-threading and acts on incoming requests from different connections concurrently. 

* NO forms of non-blocking I/O are used.

### Running the tests

test.c is the input data used to test the program and check if outputs much expected results

### Invocation
The parameters to start a station will be:•Name — the name of thıs station•Authentication file — A non-empty file, the first line of which will be used to decide if incoming connectionsare permitted.If any new connection does not supply this string as its first bytes, it is to be disconnected..•logfile — Name of a file to write station statistics to if required.•[optional] pîrt— port to listen for new connections on.•[optional] interface to listen on (defaults to localhost).For example:./station central auth1.txt t1.log 43000When a station is ready to accept connections, it should output the port number it is listening on followedby a newline to standard out.  If the station was started with no port, then use an ephemeral port.

## Project 2 - Card Game (Inter-process communication and coordination)

Two programs are run to play a card game (“clubs”).  A separate document describingthe  rules  of  “clubs”  will  be  provided.
The  first  program  (clubber)  will  listen  on  itsstdinfor  informationabout the game and give its moves tostdout..
The program will send information about the state of the gametostderr.
The  second  program  (clubhub)  will  start  a  number  of  processes  (egclubber)  to  be  players  andcommunicate with them via pipes.
The hub will be responsible for runnıng the game (sending information toplayers;processing their moves and determining the score).
The hub will also ensure that, information sent tostderrby the players, is discarded.

### Invocation
The parameters to start a player are:  
the number of players (in total) and the label of this particular player(starting at A). 
For example:./clubber 3 BThe parameters to start the hub are:
* The name of a file containing the deck ̄of cards to use.
* The number of points to play to.
* The names of programs to run as players (one for each player).  
* There must be at least two and no morethan four players.

For example:./clubhub ex.decks 10 ./clubber ./clubber ./clubberWould start a game with 3 players (each runnıng./clubber) and using the decks contained inex.decks.  Thegame will end when at least one player has 10 or more points.

### Representations

Whenever cards are represented as strings (in input files, output files, messages to user, etc) they will use twocharacters.  The first will be one of{2,3,4,5,6,7,8,9,T,J,Q,K,A}indicating the rank of the card.  The second willbe one of{S,C,D,H}indicating the suit of the card (Spades, Clubs, Diamonds, Hearts respectively).

##File with a single deck
2S,3S,4S,5S,6S,7S,8S,9S,TS,JS,QS,KS,AS
2C,3C,4C,5C,6C,7C,8C,9C,TC,JC,QC,KC,AC
2D,3D,4D,5D,6D,7D,8D,9D,TD,JD,QD,KD,AD
2H,3H,4H,5H,6H,7H,8H,9H,TH,JH,QH,KH,AH

##There is no terminating .
A file with additional decks would look like:
2S,3S,4S,5S,6C,7S,8C
9S,TS,JS,QS,KS,AS
2C,3C,4C5C,6C,7C,8C
9C,TC,JC,QC,KC,AC2D,3D,4D,5D,6D
7D,8D,9D,TD,JD,QD,KD,AD
2H
3H
4H,5H,6H,7H,8H,9H
TH,JH,QH,KH,AH
.

##Suits in order are boring
2H,3H
2S,3S,4S,5S,6C,7S,8C
TH,JH,QH,KH,AH
2C,3C,4C9S,TS,JS,QS,KS,AS
5C,6C,7C,8C
9C,TC,JC,QC,KC,AC
2D,3D,4D,5D,6D
7D,8D,9D,TD,JD,QD,KD,AD
4H,5H,6H,7H,8H,9H

### How your player will work. 
If the player is leading:
(a)  If the player has the lowest known club,  play it.  That is,  any clubs which are lower than it havealready been played.
(b)  Play the lowest available card from the suits in this order:
    •Diamonds
    •Hearts
    •Spades
    •Clubs2.  
If the player is not leading:
(a)  If the player can “follow suit”, then play the lowest available card of the lead suit.
(b)  If the player is last to play in this trick, the play the highest ranked available card from the suits inthis order:
    •Hearts
    •Diamonds
    •Clubs
    •Spades
(c)  Otherwise, play the highest ranked available card from the suits in this order:
    •Clubs
    •Diamonds
    •Hearts
    •Spades
## Built With

* Obective C
* UNIX shell scripts

## Authors

* **Julius Miyumo ** - 

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

*[Dr joel fenwick](https://github.com/joelfenwick) who was the professor teaching me this course [CSSE2310](http://uqreview.com/courses/csse2310/)
* Here is a 50 second accurate description of the course 😆. [video](https://www.youtube.com/watch?v=eJ7HP7fpnW8)  
