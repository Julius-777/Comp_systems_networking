# Train Station Network Project

### Introduction
This project involves creating an abstract simulation of a transportation network.
It will use pthreads, tcp networking and thread-safety.

### Simulation

The simulation will consist of a number of “stations”.Each station may be connected to a number of other stationsvia network connections..Network messages representing “trains” will arrive via these network connections, pickup or deposit resouces and move on to the next station.

### Implementation
The single program written is called station.There  are  no  separate  client  and  server  programs.  Each station will listen on a single TCP pîrt for connections.  All stations in a simulation willstart  off  isolated  and  listening  on  their  designated  port.   Programs  connecting  to  this  port  will  supply  anauthentication string (one line of text) followed by their station name and a sequence of trains.  The stationname will be used to forward trains to that station if required.  The program makes use of multi-threading and acts on incoming requests from different connections concurrently. 

* NO forms of non-blocking I/O are used.

### Invocation
The parameters to start a station will be:
  • Name — the name of thıs station
  • Authentication file—A non-empty file, the first line of which will be used to decide if incoming connections
  are permitted.If any new connection does not supply this string as its first bytes, it is to be disconnected..
  • logfile — Name of a file to write station statistics to if required.
  • [optional] pîrt— port to listen for new connections on.
  • [optional] interface to listen on (defaults to localhost).
For example:
./station central auth1.txt t1.log 43000
When a station is ready to accept connections, it should output the port number it is listening on followed
by a newline to standard out. If the station was started with no port, then it uses an ephemeral port.

### Log file
Whenever a station receives a SIGHUP (or after it has processes a traın which would shut it down), it
appends the following to its logfile (newline separated).
    * =======
    * The name of the station.
    * The total numbår of trains processed by the station since start.That is, the number of trains which did
    not have errors.
    * The total number of trains discarded due to arriving with at the wrong station..
    * The total number of trains disacarded due to format errors.
    * The total numbår of trains discarded due to having an invalid next station.
    * A comma separated list of stations which thıs station is currently connected to (in lexicographic order).
    If the station is connected to no other stations, output NONE here.
    * A list of resources with have been loaded or unloaded at the station (one per line) with each name followed
    by the amount loaded and unloaded. This list should be sorted lexicographically by resource name.
    * Finally, if the station has been shutdown, output either doomtrain or stopstation depending on which
    closed the station.

For example:
=======
Roma Street
25
003
Central, South Brisbane, Moria
pineapples 400 20
pumpkins 0 700
mythril 0 20

### Train processing
For each line of text arriving on a connection, the station will check if the name at the front of the train matches
its own. If not, the train will be discarded. Next, the type of train will be determined:
      * doomtrain — When this train is processed, the station will send a doomtrain message to all other stations
      it is directly connected to and then shut down. Any trailing text on a doomtrain should be ignored.
      * stopstation — When this train is processed, the station will forward the train to its next stop and then
      shut down.
      * add — gives a list of other stations this station should connect to
      * otherwise intrepret it as a list of resouces to load / unload.
Only the current payload will be processed at each station. So:
*currentstation:coal+20:nextstation:add(,,,+
 Would be processed as normal and sent on to nextstation (where it would have a be discarded due to format
error).

### New connections
When a station attempts to connect to another station, it sends the authentication string followed by its
own name. The station it has connected to will send back its name.
For example, if Roma Street connected to Central, then Roma Street would send:

secret
Roma street
and Central would send back:
Central

### Running the tests

test.c is the input data used to test the program and check if outputs much expected results

## Built With

* Obective C
* UNIX shell scripts

## Authors

* **Julius Miyumo ** - 

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

* [Dr Peter Sutton](https://uqreview.com/lecturers/peter-sutton/) who was the professor who taught the Networking part of this course [CSSE2310](http://uqreview.com/courses/csse2310/)
* Here is a 50 second accurate description of the course 😆. [video](https://www.youtube.com/watch?v=eJ7HP7fpnW8)  
