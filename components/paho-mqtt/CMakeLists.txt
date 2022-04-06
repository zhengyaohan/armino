set(incs
	client
	client/src
	packet/src
	mqtt_ui
	mqtt_ui/ssl_mqtt
	mqtt_ui/tcp_mqtt
	)
set(srcs)

if (CONFIG_PAHO_MQTT AND CONFIG_FREERTOS)
	list(APPEND srcs
		client/src/MQTTClient.c
		client/src/MQTTFreeRTOS.c
		client/paho_mqtt_udp.c
		packet/src/MQTTConnectClient.c
		packet/src/MQTTConnectServer.c
		packet/src/MQTTDeserializePublish.c
		packet/src/MQTTFormat.c
		packet/src/MQTTPacket.c
		packet/src/MQTTSerializePublish.c
		packet/src/MQTTSubscribeClient.c
		packet/src/MQTTSubscribeServer.c
		packet/src/MQTTUnsubscribeClient.c
		packet/src/MQTTUnsubscribeServer.c
		mqtt_ui/ssl_mqtt/ssl_mqtt_client_port.c
		mqtt_ui/tcp_mqtt/tcp_mqtt_client_port.c
		mqtt_ui/mqtt_client_core.c
		mqtt_ui/mqtt_client_com_port.c
	)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)
