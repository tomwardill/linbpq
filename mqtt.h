int MQTTConnect(char* host, int port, char* user, char* pass);
int MQTTPublish(void * msg, char *topic);

void MQTTKISSTX(void *message);
void MQTTKISSRX(void *message);
