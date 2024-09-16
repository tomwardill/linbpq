
#include "MQTTAsync.h"

#include "CHeaders.h"
#include "mqtt.h"

MQTTAsync client;

void onConnect(void *context, MQTTAsync_successData *response)
{
    MQTTAsync client = (MQTTAsync)context;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    int rc;

    printf("Successful MQTT connection\n");
}

void onConnectFailure(void *context, MQTTAsync_failureData *response)
{
    printf("MQTT connection failed, rc %d\n", response ? response->code : 0);
}

int MQTTPublish(void *message)
{
    MESSAGE *msg = (MESSAGE *)message;
    char From[10];
    char To[10];

    char buffer[1024];

    From[ConvFromAX25(msg->ORIGIN, From)] = 0;
    To[ConvFromAX25(msg->DEST, To)] = 0;

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

    pubmsg.payload = "ohai";
    pubmsg.payloadlen = (int)strlen("ohai ");

    char *topic;
    asprintf(&topic, "BPQ/%s/%s", From, To);

    MQTTAsync_sendMessage(client, topic, &pubmsg, &opts);
}

int MQTTConnect(char *host, int port, char *user, char *pass)
{
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    int rc;

    char *hostString;
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
