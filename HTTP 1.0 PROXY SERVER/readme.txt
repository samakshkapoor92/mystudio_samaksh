
(implementation of HTTP1.0 proxy server and client)
-----------------------------------------------------------------
Member 1 # KAPOOR, SAMAKSH
---------------------------------------

Description/Comments:
--------------------
In the simplest case, this may be accomplished via a single connection (v)
   between the user agent (UA) and the origin server (O).

       request chain ------------------------>
       UA -------------------v------------------- O
          <----------------------- response chain 

       
       

      request chain -------------------------------------->
       UA -----v----- A -----v----- B -----v----- C -----v----- O
          <------------------------------------- response chain

The figure above shows three intermediaries (A, B, and C) between the
   user agent and origin server


The following illustrates the resulting chain if B has a cached copy of an earlier response from O (via C) for a request which  has not been cached by UA or A.

          request chain ---------->
          UA -----v----- A -----v----- B - - - - - - C - - - - - - O
          <--------- response chain


Notes :
-------------------
1. HTTP proxy server and HTTP command line client was successfully implemented.
2. The implementation was for HTTP/1.0 which is specified as RFC 1945.
3. The BONUS part was also implemented for grading.
4. The complete validity of the code was checked using the test websites given.


Unix command for starting proxy server:
------------------------------------------
./proxy PROXY_SERVER_IP PROXY_SERVER_PORT

Unix command for starting client:
----------------------------------------------
./client PROXY_SERVER_IP PROXY_SERVER_PORT URL


