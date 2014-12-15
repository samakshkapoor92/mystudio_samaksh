(TFTP protocol implementation)
-----------------------------------------------------------------

Member 1 # KAPOOR, SAMAKSH 
---------------------------------------

Description/Comments:
--------------------

1.RRQ/WRQ PACKET

             2 bytes     string    1 byte     string   1 byte
            ------------------------------------------------
           | Opcode |  Filename  |   0  |    Mode    |   0  |
            ------------------------------------------------

2. DATA PACKET

                   2 bytes     2 bytes      n bytes
                   ----------------------------------
                  | Opcode |   Block #  |   Data     |
                   ----------------------------------
3. ACK PACKET
 
                           2 bytes     2 bytes
                         ---------------------
                        | Opcode |   Block #  |
                         ---------------------
4.ERROR PACKET

               2 bytes     2 bytes      string    1 byte
               -----------------------------------------
              | Opcode |  ErrorCode |   ErrMsg   |   0  |
               -----------------------------------------

Notes :
-------------------
1. TFTP protocol using UDP socket was succesfully implemented and tested for various random Ports.
2. Buffers of 512 bytes were sent from the file to be transmitted.
3. The Server was tested in the  ZACHRY lab, and it successfully transmitted packets most of the times besides minor glitches due to heavy    network.
4. Th local host testing and between 2 different computer with static IP was completely successful.


Unix command for starting server:
------------------------------------------
./server SERVER_IP SERVER_PORT

