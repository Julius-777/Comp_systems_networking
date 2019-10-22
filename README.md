# Obj_C_systems_programming_proj

## Project 1 - Cards

write two programs to play a card game (“clubs”).  A separate document describingthe  rules  of  “clubs”  will  be  provided.
The  first  program  (clubber)  will  listen  on  itsstdinfor  informationabout the game and give its moves tostdout..
The program will send information about the state of the gametostderr.
The  second  program  (clubhub)  will  start  a  number  of  processes  (egclubber)  to  be  players  andcommunicate with them via pipes.
The hub will be responsible for runnıng the game (sending information toplayers;processing their moves and determining the score).
The hub will also ensure that, information sent tostderrby the players, is discarded.

Invocation
The parameters to start a player are:  
the number of players (in total) and the label of this particular player(starting at A). 
For example:./clubber 3 BThe parameters to start the hub are:
•The name of a file containing the deck ̄of cards to use.
•The number of points to play to.
•The names of programs to run as players (one for each player).  
There must be at least two and no morethan four players.

##Project 2
