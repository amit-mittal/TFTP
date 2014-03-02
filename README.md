Trivial File Transfer Protocol(tftp)
=====================================
TFTP(RFC 1350) implementation for Networks Assignment

I. File list
------------
*tftp_c.c* - TFTP Client Side Implementation<br>
*tftp_d.c* - TFTP Server Side Implementation<br>
*utility.h*	- Utility library for the above two programs<br>
*sample* - Sample file which we would PUT/GET to/from server<br>



II. Build
---------
Simply run "make"(without quotes)


III. Command Line Arguments to Run
----------------------------------

**Run TFTP Server**<br>
$./tftp_s

**Run TFTP Client**<br>
$./tftp_c GET/PUT server_address file_name

- GET = to fetch file_name from server_address<br>
$./tftp_c GET 10.4.21.9 sample
- PUT = to send file_name to server_address<br>
$./tftp_c PUT 10.4.21.9 sample



IV. Details of Code
-------------------

**A. General**<br>
General Configuration of the program can be changed from utility.h<br>
*TIME_OUT*: It means for how much time host should wait for ACK and after that it will again send the previous packet<br>
*MAX_TRIES*: These are the maximum number of tries a host will try to send the packet to other host(client/server)<br>
*MYPORT*: It is the port number of the server

Let us suppose client wants to send a file to server. Then, if server is not able to send an ACK within TIME_OUT, then client will send the previous data packet again and will wait till it gets the ACK for that packet. It will try for MAX_TRIES and after that it will drop the connection. Also, if the ACK is getting lost and not that data packet, then the server won't write that data again to file, it will just send that ACK again. Also, we can send file of as big size from it. Individual working of the server and client has been explained in the next 2 secions. And then for the sample input and differenet TIME_OUT, output on server/client is shown in the next section.

Also if we are transfering the file to the server which already exists then server will send an ERR packet which stores the message that file already exists. Similarly, read/write priviledges and file not existing errors has been handled, server is responding to them with the proper error code and message. And once client receives error packet, it stops transfering file.

Also the packet has block size of 2 bytes and we are sending the packet as a string, even then we can transfer as large file because once the block number value crosses the limit, it again initializes to 1.<br>


**B. tftp server**<br>
This code has been implemented in file tftp_s.c

*1. Configuration*<br>
Firstly, a socket is opened on the server side and port 4950 is bound to it. This can be changed from utility.h After this, server waits for first request from the client - either RRQ or WRQ

*2. RRQ(Read Request)*<br>
After a server gets a read request with the filename, then if that file does not exist then, server throws an error packet that file does not exist. If that file exists, then server starts to send the file. Each data packet starts with the opcode 03 and then it has 2 bytes containing the block number. Then it contains the 512 bytes of data which is terminated by '\0'. And we keep on sending the packets until we reach the end of the file. We come to know about the end once the number of data bytes received are less than 512 bytes.

*3. WRQ(Write Request)*<br>
After a server gets a write request with the filename, then if that file already exists then, server throws an error packet that file already exists. If that file doesn't exist, then server sends an acknowledgement packet telling that it has successfully got the request. After this server starts to receive the data packet, it keeps on writing it to a new file until the last packet is sent.<br>


**C. tftp client**<br>
This code has been implemented in file tftp_c.c

*1. Configuration*<br>
After socket has been opened on server side on port 4950, then client sends RRQ or WRQ to the server accordingly. And then waits for the acknowledgement from server.

*2. RRQ(Read Request)*<br>
In this case, client wants to GET a file from server so, as an acknowledgement user sends the first packet and keeps on sending the subsequent ones till the whole file has been transfered. After each packet, client sends an acknowledgement packet so as to confirm the data packet received.<br>
Format - new file made is of name: name_of_file_received_client

*3. WRQ(Write Request)*<br>
In this case, client wants to PUT a file to server so, as an acknowledgement, server responds with ACK packet. But, if that file already exists on the server then, it will send en error packet, telling that file already exists. But, if that file is not there then, client keeps on sending the  data packet until the end of the file is reached. End of the file is found out when less than 512 bytes of data is sent to the server. On the server side, we keep on writing the bytes received to a new file.<br>
Format - new file made is of name: name_of_file_sent_server