# Systems Programming

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


## Built With

* Obective C
* UNIX shell scripts

## Authors

* **Julius Miyumo ** - 

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

*[Dr Peter Sutton](https://uqreview.com/lecturers/peter-sutton/) who was the professor who taught the Networking part of this course [CSSE2310](http://uqreview.com/courses/csse2310/)
* Here is a 50 second accurate description of the course 😆. [video](https://www.youtube.com/watch?v=eJ7HP7fpnW8)  
