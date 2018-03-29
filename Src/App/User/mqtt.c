#include <string.h>
#include "MQTTClient.h"


void messageArrived(MessageData* data)
{
	printf("%.*s: %.*s\n", data->topicName->lenstring.len, data->topicName->lenstring.data,
           data->message->payloadlen, data->message->payload);
}

MQTTClient client;
void vTaskMQTT( void * pvParameters )
{
    
	Network network;
    
	uint8_t sendbuf[80], readbuf[80];
	int rc = 0;
    
	MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

    NewNetwork(&network,1);
    MQTTClientInit(&client, &network, 3000,sendbuf, sizeof(sendbuf), readbuf, sizeof(readbuf));

	uint8_t address[] = {192,168,0,112};
	if ((rc = ConnectNetwork(&network, (char*)address, 1883)) != 0)
		printf("Return code from network connect is %d\n", rc);
    
#if defined(MQTT_TASK)
	if ((rc = MQTTStartTask(&client)) != pdPASS)
		printf("Return code from start tasks is %d\n", rc);
#endif
    connectData.MQTTVersion = 3;
	connectData.clientID.cstring = "FreeRTOS_Sample";

	if ((rc = MQTTConnect(&client, &connectData)) != 0)
		printf("Return code from MQTT connect is %d\n", rc);
	else
		printf("MQTT Connected\n");

	if ((rc = MQTTSubscribe(&client, "sensor", QOS1, messageArrived)) != 0)
		printf("Return code from MQTT subscribe is %d\n", rc);

    
	while (1)
	{
        
//		MQTTMessage message;
//		char payload[30];
//
//		message.qos = QOS1;
//		message.retained = 0;
//		message.payload = payload;
//		sprintf(payload, "%d", count++);
//		message.payloadlen = strlen(payload);
//
//		if ((rc = MQTTPublish(&client, "sensor", &message)) != 0)
//			printf("Return code from MQTT publish is %d\n", rc);
#if !defined(MQTT_TASK)
		if ((rc = MQTTYield(&client, 1000)) != 0)
			printf("Return code from yield is %d\n", rc);
#endif
        if(!client.isconnected)
        {
             printf("MQTT Disconnect\r\n");//MQTT Disconnect,reconnect
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
	}
}