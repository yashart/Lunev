Fifo
====

Client-Server communication with fifo

## Structures:
*This program has structures:*

* **package** - It is a transmission unit. It include client pid, package number and data. Client send this unit to server.
* **command** - It is a unit for sending information from server to client. Server send its pid and next package number.

## Functions:
*This program has functions:*

* **create fifo** - function create new fifo or doesn't create, if fifo was created earlier.
* **Client** - function which send packages to fifo. It send one package, while server doesn't send the information about receive package.
* **Server** - function which get packages from client and sending requests to client, while doesn't get new package
* Cunstructor, Destructor, Dump for all structures

##Global constants:

* FIFO_NAME - fifo name for selling packages
* COMMANDS_FIFO - fifo name for selling requests
* BUFF_SIZE - data quantity in one package
