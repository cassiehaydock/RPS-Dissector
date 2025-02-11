# Rock-Paper-Scissors Protocol (RPSP)

**Version:** 1

**Purpose:** To enable two devices to play a game of Rock-Paper-Scissors by exchanging moves and determining the winner.

## TRANSPORT CONSIDERATIONS
RPSP server listens on port 50001.

## FUNCTIONAL SPECIFICATION

### Header Format: 8 bytes

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    Version   |     Opcode    |          Game ID               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          TTL                                  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

**Version:** 1 byte

- Specifies the protocol version (e.g., 0x01 for version 1).

**Opcode:** 1 byte

- Indicates the packet type:
  1. INIT: 0x01
  2. MOVE: 0x02
  3. RESULT: 0x03
  4. ACK: 0x04
  5. ERROR: 0x05

**Game ID:** 2 byte

- A unique identifier for the game session, assigned by the initiating player.
The client generates a random ID using a random 16-bit number. If the client suspects a collision (e.g., the same Game ID already exists in the current session), it can simply regenerate the ID and retry.

**Time To Live (TTL):** 4 bytes

TTL (Time To Live) is used to ensure that packets are not processed too late due to network delays or lost packets. If a client sends a packet (such as a MOVE) and does not receive an ACK within the specified TTL, it will resend the packet to ensure the game progresses smoothly.

### Payload Format: 4 bytes

3 different payload types, depending on packet type.

#### INIT & ACK

        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                           Padding                             |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

padding a payload on packets of type INIT & ACK to make all packets the same length for easier implementation.

#### MOVE

        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                           Move                                |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  1. ROCK: 0x01
  2. PAPER: 0x02
  3. SCISSOR: 0x03

  #### RESULT

        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                           Result                              |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  1. WIN: 0x01
  2. LOSS: 0x02
  3. DRAW: 0x03

#### ERROR

        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                           Message                             |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Message: 
Error indictaing to client that something went wrong and to restart the game
END! = 0x45 0x4E 0x44 0x21

### COMMUNICATION FORMAT

#### Game Workflow

1. Client sends INIT to ask the server to start a game

    Header:

        Opcode: 0x01 (INIT)
        Message ID: 2 bytes (Unique for the game session)
        TTL: 4 bytes 

    Payload:

        Padding (The game starts with just the INIT message)

    The server stores this Unique Game session.

2. Server sends back an ACK telling the client its ready and to send a move

    Header:

        Opcode: 0x04 (ACK)
        Message ID: 2 bytes (Same as the INIT message ID, links the client’s move to the game)
        TTL: 4 bytes 

    Payload:

        Padding (The game starts with just the ACK message)

3. Client sends a MOVE 

    Header:

        Opcode: 0x02 (MOVE)
        Message ID: 2 bytes (Same as the INIT & ACK message ID, links the client’s move to the game)
        TTL: 4 bytes 

    Payload:

        Move: 1 byte (0x01 = Rock, 0x02 = Paper, 0x03 = Scissors)

    If a MOVE packet is sent before an INIT packet, the servers check for an existing GAME ID will fail. The server will ignore this packet and send an ERROR packet in response

3. Server send a RESULT back which ends the game

    Header:

        Opcode: 0x03 (RESULT)
        Message ID: 2 bytes (Same as INIT, ACK & MOVE, links the result to the game)
        TTL: 4 bytes 

    Payload:

        Result: 1 byte (0x01 = Win, 0x02 = Lose, 0x03 = Draw)

#### TTL Workflow

1. Client sends a INIT with a set TTL. If the client does not recieve an ACK from server within the set TTL, it will resend the packet.

2. After recieving an INIT the server will check the TTL of the packet to ensure it is not stale. If the TTL has expired, the server will discard the packet and send an ERROR packet. If not expired, the server will send an ACK with a TTL in response. If the server does not recive a MOVE within the TTL of the ACK packet it will end the game and send an ERROR packet.

3. The client sends a MOVE packet with a TTL, if it recieves no RESULT packet within the TTL it will resend the MOVE packet. 

4. The server receives the MOVE packet, processes it, and sends a RESULT back to the client and ends the game. The server also checks the TTL of the packet to ensure it is not stale. If the TTL has expired, the server will discard the packet and send an ERROR packet.

    For each resend of a packet the client will reduce the TTL with each resend (e.g. TTL = TTL - 1), until the TTL expires, at which point they must start a new game and the previous game will timeout on the server side and be discarded.