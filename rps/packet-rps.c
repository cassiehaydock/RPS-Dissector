#include "config.h"
#include <epan/packet.h>

// When packets are received on this port, they will be handled by this dissector.
#define RPS_PORT 50001

// static int varible that will store protocl ID
// used to register the protocol with Wireshark and to link it with the dissector function.
static int rps_proto;

// construct tables to define which fields will be present in the packet and to store the opened/closed state of the subtree.
// header fields
static int hf_rps_version;
static int hf_rps_opcode;
static int hf_rps_game_id;
static int hf_rps_ttl;
// payload types
static int hf_rps_padding;
static int hf_rps_move;
static int hf_rps_result;
static int hf_rps_message;
// tree
static int ett_rps;

//  map the raw byte values to user-friendly strings in Wireshark’s protocol display.
static const value_string rps_opcode_vals[] = {
    {0x01, "INIT"},
    {0x02, "MOVE"},
    {0x03, "RESULT"},
    {0x04, "ACK"},
    {0x05, "ERROR"},
    {0, NULL}};

static const value_string rps_move_vals[] = {
    {0x00000001, "Rock"},
    {0x00000002, "Paper"},
    {0x00000003, "Scissors"},
    {0, NULL}
};

static const value_string rps_result_vals[] = {
    {0x00000001, "Win"},
    {0x00000002, "Loss"},
    {0x00000003, "Draw"},
    {0, NULL}
};

// function is called when a packet using the "RPS" protocol is encountered.
//  It processes the packet and updates the Wireshark UI with protocol-specific information.
//  *tvb: A pointer to the "testy buffer," which contains the captured packet's data.
//  *pinfo: Contains information about the packet (e.g., protocol, source/destination, etc.).
//  *tree _U_: This argument is used to create a tree view of the protocol’s fields
//  *data _U_: Any additional data passed to the dissector
//  _U_ means field is unused
static int dissect_rps(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    // offset keeps track of where we are in the packet dissection
    int offset = 0;

    // set protocol name to "RPS" in protocl column
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "RPS");
    /* Clear the info column */
    col_clear(pinfo->cinfo, COL_INFO);

    // We can label the payload by building a subtree to decode our results into. This subtree will hold all the protocol’s details
    // the rps protocol doesn't encapsulate another protocol so we consume all of tvb data from 0 to end (-1)
    proto_item *ti = proto_tree_add_item(tree, rps_proto, tvb, 0, -1, ENC_NA);
    // add RSP subtree
    proto_tree *rps_tree = proto_item_add_subtree(ti, ett_rps);
    // add the fields to subtree
    proto_tree_add_item(rps_tree, hf_rps_version, tvb, offset, 1, ENC_BIG_ENDIAN); // version
    offset += 1;
    proto_tree_add_item(rps_tree, hf_rps_opcode, tvb, offset, 1, ENC_BIG_ENDIAN); // opcode
    guint8 opcode = tvb_get_bits8(tvb, offset * 8, 8); // extract Opcode value for payload handling
    offset += 1;
    proto_tree_add_item(rps_tree, hf_rps_game_id, tvb, offset, 2, ENC_BIG_ENDIAN); // game id
    offset += 2;
    proto_tree_add_item(rps_tree, hf_rps_ttl, tvb, offset, 4, ENC_BIG_ENDIAN); // TTL
    offset += 4;

    // Dissect the payload based on Opcode
    switch (opcode)
    {
    case 0x01: // INIT
    case 0x04: // ACK
        // INIT and ACK packets have a padded payload
        proto_tree_add_item(rps_tree, hf_rps_padding, tvb, offset, 4, ENC_NA);
        break;

    case 0x02: // MOVE
        // MOVE packets have a 4-byte "Move" field
        proto_tree_add_item(rps_tree, hf_rps_move, tvb, offset, 4, ENC_BIG_ENDIAN);
        break;

    case 0x03: // RESULT
        // RESULT packets have a 4-byte "Result" field
        proto_tree_add_item(rps_tree, hf_rps_result, tvb, offset, 4, ENC_BIG_ENDIAN);
        break;

    case 0x05: // ERROR
        // ERROR packets have a string message as the payload
        proto_tree_add_item(rps_tree, hf_rps_message, tvb, offset, 4, ENC_BIG_ENDIAN);
        break;

    default:
        // Unknown Opcode - put ERROR message
        proto_tree_add_item(rps_tree, hf_rps_message, tvb, offset, 4, ENC_BIG_ENDIAN);
        break;
    }

    // Returns the length of the captured packet.
    return tvb_captured_length(tvb);
}

// this function registers the "RPS" protocol with Wireshark
void proto_register_rps(void)
{
    // this array defines the fields that the protocol will have and how they will be displayed in Wireshark.
    // each element in this array corresponds to a field in the protocol’s packet data.
    static hf_register_info hf[] = {
        // field for "Version" (1 byte)
        // the Version field is 1 byte, so FT_UINT8 is used for unsigned 8-bit integers. It's displayed as a decimal value using BASE_DEC.
        {&hf_rps_version,
         {"Version", "rps.version", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL}},

        // field for "Opcode" (1 byte)
        // specifes type of packet - since it’s a fixed set of values, use VALS(rps_opcode_vals) to define the possible values
        {&hf_rps_opcode,
         {"Opcode", "rps.opcode", FT_UINT8, BASE_HEX, VALS(rps_opcode_vals), 0x0, NULL, HFILL}},

        // field for "Game ID" (2 bytes)
        // the Game ID is 2 bytes, so it uses FT_UINT16 (unsigned 16-bit integer) and is displayed as hexadecimal using BASE_HEX.
        {&hf_rps_game_id,
         {"Game ID", "rps.game_id", FT_UINT16, BASE_HEX, NULL, 0x0, NULL, HFILL}},

        // field for "TTL" (4 bytes)
        // the TTL is 4 bytes, represented as FT_UINT32 (unsigned 32-bit integer), and displayed in decimal (BASE_DEC)
        {&hf_rps_ttl,
         {"TTL", "rps.ttl", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL}},

         // field for "INIT" and "ACK" which is 4 butes of padding
        {&hf_rps_padding,
         {"Padding", "rps.padding", FT_UINT32, BASE_HEX, NULL, 0x0, NULL, HFILL}},

        // field for "Move" (4 byte) for MOVE packet type
        {&hf_rps_move,
         {"Move", "rps.move", FT_UINT32, BASE_HEX, VALS(rps_move_vals), 0x0, NULL, HFILL}},

        // field for "Result" (1 byte) for RESULT packet type
        {&hf_rps_result,
         {"Result", "rps.result", FT_UINT32, BASE_HEX, VALS(rps_result_vals), 0x0, NULL, HFILL}},

        // field for "Error Message" (variable length) for ERROR packet type
        {&hf_rps_message,
         {"Error Message", "rps.error_message", FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL}}};

    // this array is used to register protocol subtrees (sections of the protocol's hierarchical tree view) in Wireshark.
    // a subtree is a collection of protocol fields that are grouped together under a parent protocol tree item.
    static int *ett[] = {
        &ett_rps};

    rps_proto = proto_register_protocol(
        // full name of protocol
        "Rock Paper Scissors Protocol",
        // short name used for display in Wireshark UI
        "RPS",
        // filter name used in diaply filters (e.g. rps.port == 50001)
        "rps");

    // register the protocol fields
    // rps_proto - protocol id which links fields to correct protocol
    // hf - the array of field defintions which tells wireshark what fields belong to the "RPS" protocol and how to display them
    // array_length(gf) - tells wirehgsrak how many field sare defined in hf
    proto_register_field_array(rps_proto, hf, array_length(hf));
    // register the protocol subtree
    proto_register_subtree_array(ett, array_length(ett));
}

// adds the dissector to the Wireshark dissector table for the specified port (5001)
void proto_reg_handoff_rps(void)
{
    // store dissector handle
    static dissector_handle_t rps_handle;

    // creates the handle for dissector (dissect_rps) and associates it with the "RPS" protocl ID (rps_proto)
    rps_handle = create_dissector_handle(dissect_rps, rps_proto);
    // adds the dissector to the UDP dissector table.
    // it tells Wireshark to invoke the "RPS" protocol dissector whenever a UDP packet is received on port 50001.
    dissector_add_uint("udp.port", RPS_PORT, rps_handle);
}