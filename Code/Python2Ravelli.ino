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
		String MyText = "Stove status: ";
		
		uint8_t PreQuery[] = {0x21, 0x00, 0x10, 0x07, 0x04, 0x38, 0x95};
		uint8_t TheoricalAnswer[] = {0x11, 0x00, 0x10, 0x07, 0x04, 0xAA, 0x00, 0x00, 0xBB, 0x00, 0x00};
		uint8_t Retour[] = {5, 8};
		
		uint8_t Ans[sizeof(Retour)] = FixQuery_VarAns(PreQuery, sizeof(PreQuery), TheoricalAnswer, sizeof(TheoricalAnswer), Retour, sizeof(Retour));
		
		switch (Ans[0]) {
			case 0:
				MyText.concat("stopped");
				break;
			case 1:
				MyText.concat("fireplace cleaning");
				break;
			case 2:
				MyText.concat("waiting for pellet arrival");
				break;
			case 3:
				MyText.concat("lighting of the spark plug, arrival of the pellets");
				break;
			case 4:
				MyText.concat("vented fireplace, flame present");
				break;
			case 5:
				MyText.concat("vented fireplace, flame present");
				break;
			case 6:
				MyText.concat("frontal ventilation, work");
				break;
			case 7:
				MyText.concat("hourglass cleaning brazier");
				break;
			case 8:
				MyText.concat("during shutdown, final cleaning");
				break;
			case 9:
				MyText.concat("eco mode");
				break;
			case 10:
				MyText.concat("fault alarm");
				break;
			case 11:
				MyText.concat("Modulation");
				break;
			default:
				break;
		}
		
		MyText.concat(" - Alarme state: ");
		switch (Ans[1]) {
			case 0:
				MyText.concat("stop");
				break;
			case 1:
				MyText.concat("break");
				break;
			case 2:
				MyText.concat("in operation");
				break;
			case 3:
				MyText.concat("default");
				break;
			case 7:
				MyText.concat("cleaning");
				break;
			case 8:
				MyText.concat("pellet hatch open");
				break;
			default:
				break;
		}
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
	
	//Get checksum of request and answer
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
* Function to send a know message and receive a variable answer
*/
uint8_t FixQuery_VarAns(uint8_t Query[], uint8_t QueryL, uint8_t TAns[], uint8_t TAnsL, uint8_t Retour[], uint8_t RetL) {
	uint8_t ReceivedHexa[TAnsL];
	uint8_t AnsCkSum[TAnsL];
	uint8_t Compt = 0;
	uint8_t Escape = 0;
	uint8_t AnsData[RetL];
	
	//Get checksum of request
	Query[QueryL] = GetChkSum(Query, QueryL);
	
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
	for(uint8_t i = 0; i < TAnsL; i++) ReceivedHexa[i] = Serial.read();
	
	//Get answer checksum and compare it to stove answer
	AnsCkSum[] = GetChkSum(ReceivedHexa, TAnsL);
	if(AnsCkSum[0] = ReceivedHexa[TAnsL - 1] && AnsCkSum[0] = ReceivedHexa[TAnsL - 1]) {
		for(uint8_t i = 0; i < TAnsL; i++) {
			if(Retour[Escape] == i) { //We escape non wanted data
				AnsData[Escape] = ReceivedHexa[i].toInt();
				Escape++;
			} else {
				if(ReceivedHexa[i] != TAns[i]) Compt++;
			}
		}
		
		//Send back the status to python
		if(Compt == 0) {
			return AnsData[RetL];
		} else {
			returnThisStr("nok");
			return {0};
		}
	} else {
		returnThisStr("ERROR : incorrect stove response");
		return {0};
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