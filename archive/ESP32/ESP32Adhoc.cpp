#include "Arduino.h"
#include "ESP32Adhoc.h"

/* Comment to disable debugging here */
#define __DEBUG__ 1

void ESP32Adhoc::Debug(String str)
{
#ifdef __DEBUG__
    Serial.println(str);
#endif
}

/* 
 * Function used to check if the packet
 * is valid or not
 * returns 1 if packet begins with start byte
 * else returns zero
 */

int ESP32Adhoc::is_valid(char packet[])
{

#ifdef __DEBUG__
    Serial.print("Startbyte : ");
    Serial.println(packet[PACKET_START_BYTE_LOC], HEX);
#endif
    if (packet[PACKET_START_BYTE_LOC] == START_BYTE)
        return 1;
    else
        return 0;
}

/*
 * Following function is used for debugging purpose
 * it prints the contents of the packet 
 */

void ESP32Adhoc::print_packet(char packet[], int numBytes)
{

    Serial.println("**********************************************");

    if (packet[PACKET_START_BYTE_LOC] != START_BYTE)
    {
        Serial.println("Unknown packet format found");
        return;
    }

    Serial.print("Data length in packet : ");
    Serial.print(packet[PACKET_DATA_LENGTH_LOC], DEC);
    Serial.print(" ");
    Serial.println("bytes");

    Serial.println("|Packet contents|");
    for (int i = 0; i < numBytes; i++)
    {
        Serial.print(packet[i], HEX);
        Serial.print(" ");
    }
    Serial.println("");
    Serial.print("Source : ");
    Serial.println(ESP32Adhoc::get_src(packet));
    Serial.print("Destination : ");
    Serial.println(ESP32Adhoc::get_dst(packet));
    Serial.print("Intermediate : ");
    Serial.println(ESP32Adhoc::get_inter(packet));
    Serial.print("Counter : ");
    Serial.println(ESP32Adhoc::get_counter(packet));
    Serial.print("Current record count : ");
    Serial.println(ESP32Adhoc::record_count);
    Serial.print("Packet type : ");
    if (ESP32Adhoc::is_ACK(packet))
        Serial.println("Acknowledgement packet");
    else
        Serial.println("Forwarding packet");

    Serial.print("Length : ");
    Serial.println(packet[PACKET_DATA_LENGTH_LOC], DEC);
    Serial.print("Data : ");
    for (int i = 0; i < packet[PACKET_DATA_LENGTH_LOC]; i++)
    {
        Serial.print(packet[PACKET_DATA_LOC + i], HEX);
    }
    Serial.println(" ");
    Serial.println("**********************************************");
}

void ESP32Adhoc::print_record(unsigned int count)
{
    Serial.println("Contents of record ");
    Serial.print("Counter ");
    Serial.println(record[count].counter);
    Serial.print("src ");
    Serial.println(record[count].src, DEC);
    Serial.print("dest ");
    Serial.println(record[count].dst, DEC);
    Serial.print("Arrival time ");
    Serial.println(record[count].arrival_time, DEC);
    Serial.print("Forward ");
    Serial.println(record[count].forward, DEC);
    Serial.print("Ack ");
    Serial.println(record[count].ack, DEC);
}

int ESP32Adhoc::calc_checksum(char packet[])
{

    char checksum = packet[1];
    int length = (int)packet[6];
    for (int i = 2; i < (6 + length); i++)
    {
        checksum = checksum ^ packet[i];
    }

#ifdef __DEBUG__
    Serial.print("Calculated checksum : ");
    Serial.println(checksum, HEX);
#endif
}

int ESP32Adhoc::store_record(char packet[])
{

    if (record_count >= NUM_RECORDS)
    {
        return -1;
    }
    record[record_count].counter = ESP32Adhoc::get_counter(packet);
    record[record_count].src = ESP32Adhoc::get_src(packet);
    record[record_count].dst = ESP32Adhoc::get_dst(packet);
    record[record_count].arrival_time = millis();
    if (ESP32Adhoc::is_ACK(packet))
    {
        record[record_count].ack = 1;
    }
    else
    {
        record[record_count].forward = 1;
    }
    if (ESP32Adhoc::is_TCP(packet))
    {
        record[record_count].tcp = 1;
    }
    else
    {
        record[record_count].tcp = 0;
    }
    ESP32Adhoc::record_count++;
    return 0;
}

int ESP32Adhoc::get_table_index(int count)
{

    for (int i = 0; i < NUM_RECORDS; i++)
    {
        if (record[i].counter == count)
        {
            return i;
        }
    }
    return -1;
}

int ESP32Adhoc::can_rcv(char source)
{

#if 1
    if (con_matrix[source] == 1)
    {
        return 1;
    }
    else
    {
        return 0;
    }
#endif

#if 0
  if(connection_matrix[ID_SELF - 1][source] == 1) {
    return 1;
  }
  else {
    return 0;
  }

#endif
}
/* Checks if ACK is set in the table for
 * the count value
 */

int ESP32Adhoc::fwd_in_table(unsigned count)
{

    if (record[count].forward == 1)
    {
#ifdef __DEBUG__
        Serial.println("");
        Serial.println("In is_fwd_in_table()");
        Serial.print("forward set in table for counter : ");
        Serial.println(count);
#endif
        return 1;
    }
    else
    {
#ifdef __DEBUG__
        Serial.println("");
        Serial.println("In is_fwd_in_table()");
        Serial.print("forward not set in table for counter : ");
        Serial.println(count);
#endif
        return 0;
    }
}

int ESP32Adhoc::ack_in_table(unsigned count)
{

    if (record[count].ack == 1)
    {
#ifdef __DEBUG__
        Serial.println("");
        Serial.println("In is_ack_in_table()");
        Serial.print("ACK set in table for counter : ");
        Serial.println(count);
#endif
        return 1;
    }
    else
    {
#ifdef __DEBUG__
        Serial.println("");
        Serial.println("In is_ack_in_table()");
        Serial.print("ACK not set in table for counter : ");
        Serial.println(count);
#endif
        return 0;
    }
}
int ESP32Adhoc::ack_to_TCP(unsigned count)
{

    if (record[count].tcp == 1)
    {
#ifdef __DEBUG__
        Serial.println("");
        Serial.println("In ack_to_TCP()");
        Serial.print("TCP set in table for counter:");
        Serial.println(count);
#endif
        return 1;
    }
    else
    {
#ifdef __DEBUG__
        Serial.println("");
        Serial.println("In ack_to_TCP()");
        Serial.print("TCP not set in table for counter:");
        Serial.println(count);
#endif
        return 0;
    }
}

/* Set the ACK in the table */

void ESP32Adhoc::set_ack_table(unsigned count)
{

    record[count].ack = 1;
}

int ESP32Adhoc::is_record_present(unsigned count, char src, char dst)
{

    int i = 0;

    Serial.print("is_record_present called with Arg : ");
    Serial.println(count);
    for (i = 0; i < NUM_RECORDS; i++)
    {

        if (record[i].counter == count)
        {
            if ((record[i].src == src) & (record[i].dst == dst))
            {
                Serial.println("HIT");
                Serial.print("Record [");
                Serial.print(i);
                Serial.print("].counter = ");
                Serial.println(record[i].counter);
                Serial.println("Count = ");
                Serial.println(count);
            }
            return 1;
        }
    }
    return 0;
}

int ESP32Adhoc::is_TCP(char packet[])
{

    /* Changed during debugging */
    //return (packet[3] & (1 << 4));
    return (packet[PACKET_INTERNAL_CMD_LOC] & (1 << 4));
}

int ESP32Adhoc::get_src(char packet[])
{

    return (packet[PACKET_SRC_DST_LOC] & 0xf0) >> 4;
}
unsigned int ESP32Adhoc::get_counter(char packet[])
{

    return (packet[PACKET_COUNTER_HIGH_LOC] << 8) | (packet[PACKET_COUNTER_LOW_LOC]);
}

int ESP32Adhoc::get_dst(char packet[])
{

    return (packet[PACKET_SRC_DST_LOC] & 0x0f);
}

int ESP32Adhoc::get_inter(char packet[])
{

    return ((packet[PACKET_INTERMEDIATE_SRC_LOC] & 0xf0) >> 4);
}

int ESP32Adhoc::is_ACK(char packet[])
{

    return (packet[PACKET_INTERNAL_CMD_LOC] & 0x08) >> 3;
}

void ESP32Adhoc::set_id(char data)
{

#ifdef __DEBUG__
    Serial.print("Current SELF ID : ");
    Serial.println(ID_SELF, DEC);
#endif

    ID_SELF = data;
#ifdef __DEBUG__
    Serial.print("SELF ID changed to : ");
    Serial.println(ID_SELF, DEC);
#endif
}

long ESP32Adhoc::get_RSSI()
{
    return WiFi.RSSI();
}

int ESP32Adhoc::connect_wifi()
{
    WiFi.begin(ESP32Adhoc::ssid, ESP32Adhoc::password);
    Debug("Connecting to Wifi");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    ESP32Adhoc::wifi_status = 1;
#ifdef __DEBUG__
    Debug("WiFi connected");
    Debug("IP address: ");
    Serial.println(WiFi.localIP());
#endif
}

char ESP32Adhoc::get_self_ID(char packet[])
{

    int i;
    char len = packet[PACKET_DATA_LENGTH_LOC];
    char id;
    char *data = (char *)malloc(len * sizeof(char));
    //memcpy(data,packet+7,len);
    for (i = 0; i < len; i++)
    {
        data[i] = packet[PACKET_DATA_LOC + i];
    }
    id = data[0];
    free(data);
    return id;
}

char *ESP32Adhoc::get_ssid(char packet[])
{

    int i;
    char len = packet[6];

    char *data = (char *)malloc(len * sizeof(char));

    //memcpy(data,packet+7,len);
    for (i = 0; i < len; i++)
    {
        data[i] = packet[7 + i];
    }
#ifdef __DEBUG__
    Serial.println("Wifi SSID changed ");
#endif
    return data;
}
char *ESP32Adhoc::get_data(char packet[])
{

    int i;
    char len = packet[PACKET_DATA_LENGTH_LOC];
    char *data = (char *)malloc(len * sizeof(char));
    for (i = 0; i < len; i++)
    {
        data[i] = packet[PACKET_DATA_LOC + i];
    }
    return data;
}
char *ESP32Adhoc::get_pwd(char packet[])
{

    int i;
    char len = packet[6];
    char *data = (char *)malloc(len * sizeof(char));
    for (i = 0; i < len; i++)
    {
        data[i] = packet[7 + i];
    }
#ifdef __DEBUG__
    Serial.println("Wifi password changed ");
#endif
    return data;
}

void ESP32Adhoc::set_con_matrix(char packet[])
{

    int i;
    char len = packet[PACKET_DATA_LENGTH_LOC];
    char *data = (char *)malloc(len * sizeof(char));
    for (i = 0; i < len; i++)
    {
        data[i] = packet[PACKET_DATA_LOC + i];
    }
    ESP32Adhoc::con_matrix = data;
#ifdef __DEBUG__
    Serial.println("Connection matrix set");
    for (i = 0; i < len; i++)
    {
        Serial.print(con_matrix[i], DEC);
    }
    Serial.println("");
#endif
}

void ESP32Adhoc::set_server_ip(char packet[])
{

    int i;
    char len = packet[PACKET_DATA_LENGTH_LOC];
    unsigned char *data = (unsigned char *)malloc(len * sizeof(char));

    for (i = 0; i < len; i++)
    {
        data[i] = packet[PACKET_DATA_LOC + i];
    }

    sprintf(ESP32Adhoc::server_ip, "%d.%d.%d.%d", data[0], data[1], data[2], data[3]);
#ifdef __DEBUG__
    Serial.println("Server IP address set");
    Serial.println(ESP32Adhoc::server_ip);
    Serial.println("");
#endif
}

int ESP32Adhoc::is_internal_cmd(char packet[])
{

    char value = packet[PACKET_INTERNAL_CMD_LOC];
    char cmd = (value & 0xE0) >> 5;
#ifdef __DEBUG__
    Serial.print("Received internal Command : ");
    Serial.println(cmd, DEC);
#endif
    return cmd;
}

int ESP32Adhoc::get_index(char data[], int size, char item)
{

    for (int i = 0; i < size; i++)
    {
#ifdef __DEBUG__
        Serial.println("Data : ");
        Serial.println(data[i], HEX);
#endif
        if (data[i] == item)
        {
#ifdef __DEBUG__
            Serial.println("Index : ");
            Serial.println(i, DEC);
#endif
            return i;
        }
    }

    return -1;
}

void ESP32Adhoc::extract_ssid_pwd(char packet[], int size)
{

    int delim_index = -1;
    int data_length = (int)packet[PACKET_DATA_LENGTH_LOC];

    char *data = get_data(packet);

#ifdef __DEBUG__
    Serial.println("Data obtained :");
    for (int j = 0; j < data_length; j++)
    {
        Serial.print(data[j], HEX);
        Serial.print(" ");
    }
#endif

    delim_index = get_index(data, data_length, 0xA9);
#ifdef __DEBUG__
    Serial.print("Delimiter index : " + delim_index);
    Serial.println(delim_index, DEC);
#endif

    char *wifi_ssid = (char *)malloc((delim_index + 1) * sizeof(char));
    //char *wifi_ssid = (char *)malloc((delim_index ) * sizeof(char));
    memset(wifi_ssid, '\0', (delim_index + 1) * sizeof(char));
    //memset(wifi_ssid,'\0',(delim_index) * sizeof(char));
    memcpy(wifi_ssid, data, delim_index);
#ifdef __DEBUG__
    Serial.print("New SSID : ");
    Serial.println(wifi_ssid);
#endif

    char *wifi_pwd = (char *)malloc(((data_length - (delim_index))) * sizeof(char));
    memset(wifi_pwd, '\0', ((data_length - (delim_index))) * sizeof(char));
    for (int i = 0; i < ((data_length - delim_index) - 1); i++)
    {
        wifi_pwd[i] = data[delim_index + 1 + i];
    }
#ifdef __DEBUG__
    Serial.print("New PWD : ");
    Serial.println(wifi_pwd);
#endif
    //memcpy(wifi_pwd,data + delim_index+1,(data_length -(delim_index )));

#ifdef __DEBUG__
    Serial.println("Setting SSID and password");
#endif
    ESP32Adhoc::ssid = wifi_ssid;
    ESP32Adhoc::password = wifi_pwd;
#ifdef __DEBUG__
    Serial.print("SSID set to : ");
    Serial.println(ESP32Adhoc::ssid);
    Serial.print("Password set to : ");
    Serial.println(ESP32Adhoc::password);
#endif
}

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

void ESP32Adhoc::process_internal_cmd(char packet[])
{

    int cmd = is_internal_cmd(packet);
    char len = packet[PACKET_DATA_LENGTH_LOC];
    char data;
    long RSSI;

    switch (cmd)
    {
    case INTERNAL_SET_ID:
        data = get_self_ID(packet);
        set_id(data);
        break;

    case INTERNAL_SET_SSID_PWD:

        /* Will be handled in arduino loop itself */
        //ESP32Adhoc::ssid = get_ssid(packet);
        // extract_ssid_pwd(packet,ESP32Adhoc::recvBytes);
#ifdef __DEBUG__
        Serial.println("Initiating connection to WiFi");
#endif
        //connect_wifi();
#ifdef __DEBUG__
        Serial.println("Done");
#endif
        //delay(1000);
        break;
    case INTERNAL_SET_MATRIX:
        set_con_matrix(packet);
        break;
    case INTERNAL_GET_RSSI:
        // serial.send(packet,sizeof(packet));
        /* Handle RSSI will be taken care in loop itself */
        break;
    case INTERNAL_SET_SERVER_IP:
        set_server_ip(packet);
        break;
    case INTERNAL_NON_DEMO_MODE:
        DEMO_MODE = 1;
        break;
    default:
        Serial.println("Unknown command");
    }
}
