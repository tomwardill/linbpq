
#include "MQTTAsync.h"
#include <jansson.h>

#include "CHeaders.h"
#include "asmstrucs.h"
#include "bpqmail.h"
#include "mqtt.h"

MQTTAsync client;

void onConnect(void* context, MQTTAsync_successData* response)
{
        MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
        MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
        int rc;

        printf("Successful MQTT connection\n");

	// Send start up message
	char NodeCall[11];
	char * ptr;
	memcpy(NodeCall, MYNODECALL, 10);

	ptr=strchr(NodeCall, ' ');
	if (ptr) *(ptr) = 0;
	char *topic;
	asprintf(&topic, "PACKETNODE/%s", NodeCall);
	pubmsg.payload = "{\"status\":\"online\"}";
	pubmsg.payloadlen = strlen(pubmsg.payload);

	MQTTAsync_sendMessage(client, topic, &pubmsg, &opts)
}

void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
        printf("MQTT connection failed, rc %d\n", response ? response->code : 0);
}

char* jsonEncodeMessage(MESSAGE *msg) {
    char From[10];
    char To[10];
    From[ConvFromAX25(msg->ORIGIN, From)] = 0;
    To[ConvFromAX25(msg->DEST, To)] = 0;

    json_t *root = json_object();
    json_object_set_new(root, "from", json_string(From));
    json_object_set_new(root, "to", json_string(To));

    char buffer[1024];
    unsigned long long SaveMMASK = MMASK;
    BOOL SaveMTX = MTX;
    BOOL SaveMCOM = MCOM;
    BOOL SaveMUI = MUIONLY;
    IntSetTraceOptionsEx(8, TRUE, TRUE, FALSE);
    int len = IntDecodeFrame(msg, buffer, &msg->Timestamp, 1, FALSE, FALSE);
    IntSetTraceOptionsEx(SaveMMASK, SaveMTX, SaveMCOM, SaveMUI);

    json_object_set_new(root, "payload", json_string(buffer));
    json_object_set_new(root, "port", json_integer(msg->PORT));
    char payload_timestamp[8];
    struct tm * TM = localtime(&msg->Timestamp);
    sprintf(payload_timestamp, "%02d:%02d:%02d", TM->tm_hour, TM->tm_min, TM->tm_sec);
    json_object_set_new(root, "timestamp", json_string(payload_timestamp));

    char *msg_str = json_dumps(root, 0);
    json_decref(root);
    return msg_str;
}

void MQTTKISSTX(void *message)
{
    MESSAGE *msg = (MESSAGE *)message;
    char NodeCall[11];
    char * ptr;
    memcpy(NodeCall, MYNODECALL, 10);

	ptr=strchr(NodeCall, ' ');
	if (ptr) *(ptr) = 0;

    char *topic;
    asprintf(&topic, "PACKETNODE/ax25/trace/bpqformat/%s/sent/%d", NodeCall, msg->PORT);

    char *msg_str = jsonEncodeMessage(msg);

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

    pubmsg.payload = msg_str;
    pubmsg.payloadlen = strlen(msg_str);

    MQTTAsync_sendMessage(client, topic, &pubmsg, &opts);
}

void MQTTKISSTX_RAW(char* buffer, int bufferLength, void* PORT) {
    PPORTCONTROL PPORT = (PPORTCONTROL)PORT;
    char NodeCall[11];
    char * ptr;
    memcpy(NodeCall, MYNODECALL, 10);

    ptr=strchr(NodeCall, ' ');
    if (ptr) *(ptr) = 0;

    char *topic;
    asprintf(&topic, "PACKETNODE/kiss/%s/sent/%d", NodeCall, PPORT->PORTNUMBER);

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

    // This can container null, so we need to know
    // the exact length of the buffer
    pubmsg.payload = buffer;
    pubmsg.payloadlen = bufferLength;

    MQTTAsync_sendMessage(client, topic, &pubmsg, &opts);
}


void MQTTKISSRX(void *message)
{
    MESSAGE *msg = (MESSAGE *)message;
    char NodeCall[11];
    char * ptr;
    memcpy(NodeCall, MYNODECALL, 10);

	ptr=strchr(NodeCall, ' ');
	if (ptr) *(ptr) = 0;
    char *topic;
    asprintf(&topic, "PACKETNODE/ax25/trace/bpqformat/%s/rcvd/%d", NodeCall, msg->PORT);
    char *msg_str = jsonEncodeMessage(msg);

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

    pubmsg.payload = msg_str;
    pubmsg.payloadlen = strlen(msg_str);

    MQTTAsync_sendMessage(client, topic, &pubmsg, &opts);
}

void MQTTKISSRX_RAW(char* buffer, int bufferLength, void* PORT) {
    PPORTCONTROL PPORT = (PPORTCONTROL)PORT;
    char NodeCall[11];
    char * ptr;
    memcpy(NodeCall, MYNODECALL, 10);

    ptr=strchr(NodeCall, ' ');
    if (ptr) *(ptr) = 0;

    char *topic;
    asprintf(&topic, "PACKETNODE/kiss/%s/rcvd/%d", NodeCall, PPORT->PORTNUMBER);

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

    // This can container null, so we need to know
    // the exact length of the buffer
    pubmsg.payload = buffer;
    pubmsg.payloadlen = bufferLength;

    MQTTAsync_sendMessage(client, topic, &pubmsg, &opts);
}

char* replace(char* str, char* a, char* b)
{
    int len  = strlen(str);
    int lena = strlen(a), lenb = strlen(b);
    for (char* p = str; p = strstr(p, a); ++p) {
        if (lena != lenb) // shift end as needed
            memmove(p+lenb, p+lena,
                len - (p - str) + lenb);
        memcpy(p, b, lenb);
    }
    return str;
}

int MQTTPublish(void *message, char *topic)
{
    MESSAGE *msg = (MESSAGE *)message;
    char From[10];
    char To[10];

    char buffer[1024];

    From[ConvFromAX25(msg->ORIGIN, From)] = 0;
    To[ConvFromAX25(msg->DEST, To)] = 0;

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

    time_t timestamp = msg->Timestamp;

    unsigned long long SaveMMASK = MMASK;
    BOOL SaveMTX = MTX;
    BOOL SaveMCOM = MCOM;
    BOOL SaveMUI = MUIONLY;

    IntSetTraceOptionsEx(8, TRUE, TRUE, FALSE);
    int len = IntDecodeFrame(msg, buffer, timestamp, 1, FALSE, FALSE);
    IntSetTraceOptionsEx(SaveMMASK, SaveMTX, SaveMCOM, SaveMUI);

    // MQTT _really_ doesn't like \r, so replace it with something
    // that is at least human readable
    char* replaced_buffer = replace(buffer, "\r", "\r\n");

    pubmsg.payload = replaced_buffer;
    pubmsg.payloadlen = strlen(replaced_buffer);

    printf("%s\n", replaced_buffer);

    MQTTAsync_sendMessage(client, topic, &pubmsg, &opts);
}

int MQTTConnect(char* host, int port, char* user, char* pass)
{
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    int rc;

    char* hostString;
    asprintf(&hostString, "tcp://%s:%d", host, port);

    rc = MQTTAsync_create(&client, hostString, "BPQ", MQTTCLIENT_PERSISTENCE_NONE, NULL);

    if (rc != MQTTASYNC_SUCCESS)
    {
        printf("Failed to create client, return code %d\n", rc);
        return rc;
    }

    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = user;
    conn_opts.password = pass;
    conn_opts.onSuccess = onConnect;
    conn_opts.onFailure = onConnectFailure;

    rc = MQTTAsync_connect(client, &conn_opts);

    if (rc != MQTTASYNC_SUCCESS)
    {
        printf("Failed to start connect, return code %d\n", rc);
        return rc;
    }

    return 0;
}

void MQTTMessageEvent(void* message) {
    struct MsgInfo* msg = (struct MsgInfo *)message;

    json_t *root = json_object();
    json_object_set_new(root, "id", json_integer(msg->number));
    json_object_set_new(root, "size", json_integer(msg->length));
    json_object_set_new(root, "type", json_string(msg->type == 'P' ? "P" : "B"));
    json_object_set_new(root, "to", json_string(msg->to));
    json_object_set_new(root, "from", json_string(msg->from));
    json_object_set_new(root, "subj", json_string(msg->title));

    switch(msg->status) {
        case 'N':
            json_object_set_new(root, "event", json_string("newmsg"));
            break;
        case 'F':
            json_object_set_new(root, "event", json_string("fwded"));
            break;
        case 'R':
            json_object_set_new(root, "event", json_string("read"));
            break;
        case 'K':
            json_object_set_new(root, "event", json_string("killed"));
            break;
    }

    char *msg_str = json_dumps(root, 0);

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

    pubmsg.payload = msg_str;
    pubmsg.payloadlen = strlen(msg_str);

    char NodeCall[11];
    char * ptr;
    memcpy(NodeCall, MYNODECALL, 10);

    ptr=strchr(NodeCall, ' ');
    if (ptr) *(ptr) = 0;

    char *topic;
    asprintf(&topic, "PACKETNODE/event/%s/pmsg", NodeCall);

    MQTTAsync_sendMessage(client, topic, &pubmsg, &opts);
}
