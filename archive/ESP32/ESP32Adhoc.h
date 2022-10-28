
#ifndef ESP32Adhoc_h
#define ESP32Adhoc_h
#include <WiFi.h>
#include <WiFiUdp.h>
#include "Arduino.h"
#include "matrix.h"


#define ADHOC_UDP_PORT 5000
#define TCP_PORT 5050

/* Packet format details */


#define START_BYTE 0xFF

#define PACKET_START_BYTE_LOC 0
#define PACKET_SRC_DST_LOC 1
#define PACKET_INTERMEDIATE_SRC_LOC 2
#define PACKET_INTERNAL_CMD_LOC 3
#define PACKET_COUNTER_HIGH_LOC 4
#define PACKET_COUNTER_LOW_LOC 5
#define PACKET_DATA_LENGTH_LOC 6
#define PACKET_DATA_LOC 7

/* Process the internal commands between
 * Arduino and ESP.
 * 3-bits
 *
 * |CMD|Desription|
 * |000|No internal command 
 * |001|Set the ID of the bot 
 * |010|Set ssid and password of wifi 
 * |011|get init_matrix for bot ID 
 * |100|Get RSSI 
 * |101|Server IP set
 * |110|Reserved 
 * |111|Reserved
 *
 */

/* List of internal commands */

#define INTERNAL_SET_ID 1 
#define INTERNAL_SET_SSID_PWD 2 
#define INTERNAL_SET_MATRIX 3 
#define INTERNAL_GET_RSSI 4 
#define INTERNAL_SET_SERVER_IP 5
#define INTERNAL_NON_DEMO_MODE 6


/* Number of records to maintain */
#define NUM_RECORDS 512

class ESP32Adhoc 
{
    public:
    
        /* ID of the bot */
        char ID_SELF;
        /* Wifi and pssword */
        char* ssid ;
        char* password;
        char *con_matrix;
        /* Connection matrix row for the ID */
        int wifi_status = 0;

        /* To Enable or disable demo mode 
         * (Disabled by default)
         */

        int DEMO_MODE = 0;
 
        /* TCP server ip address */
        char server_ip[16];
        /* Global variable used to maintain current record count */
        unsigned int record_count = 0;
    
        /* To keep track of received bytes from UDP */

        int recvBytes;

        /* Table used to record the movement of packets in network */

        struct table {
            unsigned int counter;
            char src;
            char dst;
            unsigned long arrival_time;
            char forward:1;
            char ack:1;
            char tcp:1;
            char valid:1;
        };

        struct table record[NUM_RECORDS];
       
        /* Constructor */

        /* Start Adhoc mode on the specified port */
        void start();

        int parseBytes();

        /* For debugging purposes */
        void print_packet(char packet[],int numBytes); 
        void Debug(String str);
        void print_record(unsigned int count);
        int is_valid(char packet[]); 

        /* Database storage */ 
        int store_record(char packet[]);
        int is_record_present(unsigned count, char src, char dst);
        void set_ack_table(unsigned count);
        int ack_in_table(unsigned count);
        int ack_to_TCP(unsigned count);
        int fwd_in_table(unsigned count);
        int get_table_index(int count);
        int get_src(char packet[]);
        int get_dst(char packet[]);
        unsigned int get_counter(char packet[]);
        int get_inter(char packet[]);
        int is_ACK(char packet[]);
        int is_TCP(char packet[]);
        int can_rcv(char src);

        /* For Checksum calculation */
        int calc_checksum(char packet[]);

        /* To handle internal commands */
        int is_internal_cmd(char packet[]);
        void process_internal_cmd(char packet[]);
        char *get_pwd(char packet[]);
        char *get_data(char packet[]);
        char *get_ssid(char packet[]);
        void set_con_matrix(char packet[]);
        void set_server_ip(char packet[]);
        char get_self_ID(char packet[]);
        int connect_wifi();
        long get_RSSI();
        void set_id(char data);
        

        /* Adhoc reception logic */
        void adhoc_recv_filter(char packet[]);

        /* Adhoc send logic */
        void adhoc_send_filter(char packet[],int numBytes);
        void extract_ssid_pwd(char packet[],int recvBytes);
       
    private:
    
        /* port used by the adhoc network nodes */
        int adhoc_port;
        /* Function to extract ssid and password from packet */
        int get_index(char data[],int size,char item); 
};

#endif
