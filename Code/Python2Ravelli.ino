/*
* Written by DuchkPy
* https://github.com/DuchkPy/Python2Ravelli/
* 
* Created to interface a Ravelli stove with an ESP8266 board
* List of compatible stove models available on github
*/
#include "ESP_MICRO.h"
#include "Crc16.h"

void setup() {
	Serial.begin(4800);
	start("WIFI_SSID", "PASSWORD"); //Connect to your wifi
}

void loop() {
	//Waits new orders from external python request
	waitUntilNewReq();

	//Local variables
	String ReqType = "";
	uint8_t ReqValue = 0;

	//Get request type and if it exist the requested value
	if(getPath().indexOf("-") >= 0) {
		ReqType = getPath().substring(0, getPath().indexOf("-"));
		ReqValue = getPath().substring(getPath().indexOf("-") + 1).toInt();
	} else {
		ReqType = getPath();
	}

	//Get the job done !
	if(ReqType == "RoomTemp") { //Send room temperature (°C)
		uint8_t PreQuery[] = {0x21, 0x00, 0x11, 0x00, 0x01, ReqValue, 0x00, 0x00};
		uint8_t TheoricalAnswer[] = {0x11, 0x00, 0x11, 0x00, 0x01, ReqValue, 0x00, 0x00};

		VarQuery_FixAns(PreQuery, sizeof(PreQuery), TheoricalAnswer, sizeof(TheoricalAnswer));
	}
	else if(ReqType == "SetpointTemp") { //Send setpoint temperature (°C)
		uint8_t PreQuery[] = {0x21, 0x00, 0x02, 0x00, 0x53, ReqValue, 0x00, 0x00};
		uint8_t TheoricalAnswer[] = {0x11, 0x00, 0x02, 0x00, 0x53, ReqValue, 0x00, 0x00};

		VarQuery_FixAns(PreQuery, sizeof(PreQuery), TheoricalAnswer, sizeof(TheoricalAnswer));
	}
	else if(ReqType == "HeatingPower") { //Send heating power (1 to 5)
		if(ReqValue >= 1 && ReqValue <= 5) {
			uint8_t PreQuery[] = {0x21, 0x00, 0x02, 0x00, 0x52, ReqValue, 0x00, 0x00};
			uint8_t TheoricalAnswer[] = {0x11, 0x00, 0x02, 0x00, 0x52, ReqValue, 0x00, 0x00};

			VarQuery_FixAns(PreQuery, sizeof(PreQuery), TheoricalAnswer, sizeof(TheoricalAnswer));
		} else {
			returnThisStr("ERROR : Heating power can only be an integer between 1 and 5");
		}
	}
	else if(ReqType == "FanPower") { //Send from fan power (0 to 6)
		if(ReqValue >= 0 && ReqValue <= 6) {
			uint8_t PreQuery[] = {0x21, 0x00, 0x02, 0x00, 0x58, ReqValue, 0x00, 0x00};
			uint8_t TheoricalAnswer[] = {0x11, 0x00, 0x02, 0x00, 0x58, ReqValue, 0x00, 0x00};

			VarQuery_FixAns(PreQuery, sizeof(PreQuery), TheoricalAnswer, sizeof(TheoricalAnswer));
		} else {
			returnThisStr("ERROR : Fan power can only be an integer between 0 and 6");
		}
	}
	else if(ReqType == "StoveStatus") { //Request stove status
	}
	else if(ReqType == "OnOff") { //Switch ON or OFF
	}
	else if(ReqType == "SetpointTempStatus") { //Request registered setpoint temperature (°C)
	}
	else if(ReqType == "HeatingPowerStatus") { //Request current heating power
	}
	else if(ReqType == "FanPowerStatus") { //Request current fan power
	}
	else if(ReqType == "AlarmList") { //Request alarm list
	}
	else returnThisStr("ERROR : request not recognized");
}

/*
* Function to send a variable message and receive a known answer
*/
void VarQuery_FixAns(uint8_t Query[], uint8_t QueryL, uint8_t TAns[], uint8_t TAnsL) {
	uint8_t Compt = 0;
	String MaVariable1 = "";
	String MaVariable2 = "";

	//Get checksum of request
	Query[QueryL] = GetChkSum(Query, QueryL);
	TAns[TAnsL] = GetChkSum(TAns, TAnsL);

	//Purge remaining data on buffer
	while (Serial.available() > 0) {
		Serial.read();
	}

	//Send query to stove and wait for it's answer
	Serial.write(Query, QueryL);
	Serial.flush();
	delay(50);

	//Get answer from stove and compare to theoritical answer
	for(uint8_t i = 0; i < QueryL; i++) Serial.read();
	for(uint8_t i = 0; i < TAnsL; i++) {
		if(Serial.read() != TAns[i]) Compt++;
	}

	//Send back the status to python
	if(Compt == 0) {
		returnThisStr("ok");
	} else {
		returnThisStr("nok");
	}
}

/*
* Function to calculate checksum and return complete frame
*/
uint8_t GetChkSum(uint8_t frame[], uint8_t length) {
	Crc16 crc;
	crc.clearCrc();
	uint16_t TtlChkSum;
	//Get checksum
	TtlChkSum = crc.XModemCrc(frame, 0, length - 2);
	//Complete frame
	frame[length - 1] = TtlChkSum >> 0;
	frame[length - 2] = TtlChkSum >> 8;

	return frame[length];
}