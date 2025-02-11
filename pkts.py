from scapy.all import *

# configure Scapy to use L3RawSocket for sending packets
# from https://stackoverflow.com/questions/75312602/sending-packet-with-python-and-scapy-to-local-address-doesnt-work
conf.L3socket = L3RawSocket

# I am sending to my own laptop so set dest ip localhost
dest_ip = "127.0.0.1"
# the server is listening on port 50001 so the dest port is 50001
dest_port = 50001

# version
version = (0x01).to_bytes(1, byteorder='big')      # protocol version [1 byte]
ttl = (0x3C).to_bytes(4, byteorder='big')          # time to live (60 second) [4 bytes]
# multiple game ID's
gameIDs = [0x1234, 0x1212]
# opcode options
opcodes = [0x01, 0x02, 0x03, 0x04, 0x05]

packets = []
# for each different game
for gameID in gameIDs:
    # generate different packet type headers
    for opcode in opcodes:
        header = version + (opcode).to_bytes(1, byteorder='big') + (gameID).to_bytes(2, byteorder='big') + ttl

        # depending on opcode make different payloads
        match opcode:
            # INIT or ACK
            case 0x01 | 0x04:
                # paylod of 4 bytes of padding
                payload = (0x00).to_bytes(4, byteorder='big')
                # make the packet and add to array of packets
                pkt = header + payload
                packets.append(pkt)
            # MOVE or RESULT
            case 0x02 | 0x03:
                # diff options for moves and results
                options = [0x01, 0x02, 0x03]
                # payload of each type + 3 bytes of padding
                for op in options:
                    payload = (op).to_bytes(4, byteorder='big')
                    # make the packet and add to array of packets
                    pkt = header + payload
                    packets.append(pkt)
            # ERROR
            case 0x05:
                # payload of ASCCI for "END!" error
                payload = bytes([0x45, 0x4E, 0x44, 0x21])
                # make the packet and add to array of packets
                pkt = header + payload
                packets.append(pkt)
            case _:
                print("Unrecognized opcode")


# # debugging printing
# for pkt in packets:
#     print(f"rpsp_packet size: {len(pkt)}")
#     print(f"rpsp_packet: {pkt}")

# Create IP/TCP packet
ip_layer = IP(dst=dest_ip)
udp_layer = UDP(dport=dest_port, sport=RandShort())

# send the packets
for pkt in packets:
    send(ip_layer / udp_layer / pkt)

# send packet to signigy end of transmission
# send(ip_layer / udp_layer / "EXIT")

