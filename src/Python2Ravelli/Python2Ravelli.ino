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
  Serial.swap();
  start("WIFI_SSID", "PASSWORD"); //Connect to your wifi
}

void loop() {
  // Waits new orders from external python request
  waitUntilNewReq();

  // Local variables
  String ReqType = "";
  uint8_t ReqValue = 0;

  // Get request type and if it exist the requested value
  if (getPath().indexOf("-") >= 0) {
    ReqType = getPath().substring(0, getPath().indexOf("-"));
    ReqValue = getPath().substring(getPath().indexOf("-") + 1).toInt();
  } else {
    ReqType = getPath();
  }

  /** Get the job done ! **/
  // Action items:
  if (ReqType == "OnOff") { // Switch ON or OFF
    uint8_t PreQuery[] = {0x21, 0x00, 0x07, 0x01, 0xC8, 0x4C};
    uint8_t TheoricalReply[] = {0x11, 0x00, 0x07, 0x01, 0x00, 0x18, 0xAA};

    FixQuery_FixReply(PreQuery, sizeof(PreQuery), TheoricalReply, sizeof(TheoricalReply));
  }
  else if (ReqType == "RoomTemp") { // Send room temperature (°C)
    if (ReqValue >= 0 && ReqValue <= 99) {
      uint8_t PreQuery[] = {0x21, 0x00, 0x11, 0x00, 0x01, ReqValue, 0x00, 0x00};
      uint8_t TheoricalReply[] = {0x11, 0x00, 0x11, 0x00, 0x01, 0xCA, 0x79};

      VarQuery_FixReply(PreQuery, sizeof(PreQuery), TheoricalReply, sizeof(TheoricalReply));
    } else {
      returnThisStr("ERROR: Maximum 2 digit number allowed");
    }
  }
  else if (ReqType == "SetpointTemp") { // Send setpoint temperature (°C)
    if (ReqValue >= 0 && ReqValue <= 99) {
      uint8_t PreQuery[] = {0x21, 0x00, 0x02, 0x00, 0x53, ReqValue, 0x00, 0x00};
      uint8_t TheoricalReply[] = {0x11, 0x00, 0x02, 0x00, 0x53, ReqValue, 0x00, 0x00};

      VarQuery_FixReply(PreQuery, sizeof(PreQuery), TheoricalReply, sizeof(TheoricalReply));
    } else {
      returnThisStr("ERROR: Maximum 2 digit number allowed");
    }
  }
  else if (ReqType == "HeatingPower") { // Send heating power (1 to 5)
    if (ReqValue >= 1 && ReqValue <= 5) {
      uint8_t PreQuery[] = {0x21, 0x00, 0x02, 0x00, 0x52, ReqValue, 0x00, 0x00};
      uint8_t TheoricalReply[] = {0x11, 0x00, 0x02, 0x00, 0x52, ReqValue, 0x00, 0x00};

      VarQuery_FixReply(PreQuery, sizeof(PreQuery), TheoricalReply, sizeof(TheoricalReply));
    } else {
      returnThisStr("ERROR: Heating power can only be an integer between 1 and 5");
    }
  }
  else if (ReqType == "FanPower") { // Send from fan power (0 to 6)
    if (ReqValue >= 0 && ReqValue <= 6) {
      uint8_t PreQuery[] = {0x21, 0x00, 0x02, 0x00, 0x58, ReqValue, 0x00, 0x00};
      uint8_t TheoricalReply[] = {0x11, 0x00, 0x02, 0x00, 0x58, ReqValue, 0x00, 0x00};

      VarQuery_FixReply(PreQuery, sizeof(PreQuery), TheoricalReply, sizeof(TheoricalReply));

      uint8_t PreQuery2[] = {0x21, 0x00, 0x10, 0x13, 0x01, 0xA7, 0x87};
      uint8_t TheoricalReply2[] = {0x11, 0x00, 0x10, 0x13, 0x01, 0x00, 0x6D, 0x81};
      FixQuery_FixReply(PreQuery2, sizeof(PreQuery2), TheoricalReply2, sizeof(TheoricalReply2));
    } else {
      returnThisStr("ERROR: Fan power can only be an integer between 0 and 6");
    }
  }
  else if (ReqType == "ScrewLoading") { // Request to empty the fireplace and fill it with fresh pellet
    uint8_t PreQuery[] = {0x21, 0x00, 0x07, 0x08, 0x59, 0x65};
    uint8_t TheoricalReply[] = {0x11, 0x00, 0x07, 0x08, 0xAA, 0x00, 0x00};
    uint8_t Reply[sizeof(TheoricalReply)];

    FixQuery_VarReply(PreQuery, sizeof(PreQuery));
    for (uint8_t i = 0; i < sizeof(PreQuery); i++) Serial.read();
    for (uint8_t i = 0; i < sizeof(TheoricalReply); i++) Reply[i] = Serial.read();

    if (ConfirmChkSum(Reply, sizeof(TheoricalReply))) {
      if (Reply[4] == 0) {
        returnThisStr("Start of screw loading");
      }
      else if (Reply[4] == 1) {
        returnThisStr("Screw loading impossible");
      }
    } else {
      returnThisStr("ERROR: incorrect stove response");
    }

    // AA = 00 --> possible ; 01 --> impossible (par exemple poêle en cours de fonctionnement)
    // Possibilité d'avoir la du durée du temps de chargement et/ou le temps restant ???
  }
  // Information items:
  else if (ReqType == "StoveStatus") { // Request stove status
    String MyText = "Stove status: ";

    uint8_t PreQuery[] = {0x21, 0x00, 0x10, 0x07, 0x04, 0x38, 0x95};
    uint8_t TheoricalReply[] = {0x11, 0x00, 0x10, 0x07, 0x04, 0xA1, 0x00, 0x00, 0xB1, 0x00, 0x00};
    uint8_t Reply[sizeof(TheoricalReply)];

    FixQuery_VarReply(PreQuery, sizeof(PreQuery));
    for (uint8_t i = 0; i < sizeof(PreQuery); i++) Serial.read();
    for (uint8_t i = 0; i < sizeof(TheoricalReply); i++) Reply[i] = Serial.read();
    if (ConfirmChkSum(Reply, sizeof(TheoricalReply))) {
      if (Reply[0] == 0) {
        returnThisStr("ERROR: incorrect stove response");
      } else {

        switch (Reply[5]) {
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
          MyText.concat("status not documented");
          break;
        case 5:
          MyText.concat("vented fireplace, flame present");
          break;
        case 6:
          MyText.concat("work");
          break;
        case 7:
          MyText.concat("hourglass ; cleaning fireplace");
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
        switch (Reply[8]) {
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

        returnThisStr(MyText);
      }
    } else {
      returnThisStr("ERROR: incorrect stove response");
    }


  }
  else if (ReqType == "SetpointTempStatus") { // Request registered setpoint temperature (°C)
    uint8_t PreQuery[] = {0x21, 0x00, 0x01, 0x00, 0x53, 0xFF, 0x43};
    uint8_t TheoricalReply[] = {0x11, 0x00, 0x01, 0x00, 0x53, 0x1f, 0x07, 0x29, 0xAA, 0x00, 0x00};
    uint8_t Reply[sizeof(TheoricalReply)];

    FixQuery_VarReply(PreQuery, sizeof(PreQuery));
    for (uint8_t i = 0; i < sizeof(PreQuery); i++) Serial.read();
    for (uint8_t i = 0; i < sizeof(TheoricalReply); i++) Reply[i] = Serial.read();

    if (ConfirmChkSum(Reply, sizeof(TheoricalReply))) {
      returnThisStr(String(Reply[8], DEC));
    } else {
      returnThisStr("ERROR: incorrect stove response");
    }
  }
  else if (ReqType == "HeatingPowerStatus") { // Request current heating power
    uint8_t PreQuery[] = {0x21, 0x00, 0x01, 0x00, 0x52, 0xEF, 0x62};
    uint8_t TheoricalReply[] = {0x11, 0x00, 0x01, 0x00, 0x52, 0x05, 0x01, 0x05, 0xAA, 0x00, 0x00};
    uint8_t Reply[sizeof(TheoricalReply)];

    FixQuery_VarReply(PreQuery, sizeof(PreQuery));
    for (uint8_t i = 0; i < sizeof(PreQuery); i++) Serial.read();
    for (uint8_t i = 0; i < sizeof(TheoricalReply); i++) Reply[i] = Serial.read();

    if (ConfirmChkSum(Reply, sizeof(TheoricalReply))) {
      returnThisStr(String(Reply[8], DEC));
    } else {
      returnThisStr("ERROR: incorrect stove response");
    }
  }
  else if (ReqType == "FanPowerStatus") { // Request current fan power
    uint8_t PreQuery[] = {0x21, 0x00, 0x01, 0x00, 0x58, 0x4E, 0x28};
    uint8_t TheoricalReply[] = {0x11, 0x00, 0x01, 0x00, 0x58, 0x22, 0x00, 0x06, 0xAA, 0x00, 0x00};
    uint8_t Reply[sizeof(TheoricalReply)];

    FixQuery_VarReply(PreQuery, sizeof(PreQuery));
    for (uint8_t i = 0; i < sizeof(PreQuery); i++) Serial.read();
    for (uint8_t i = 0; i < sizeof(TheoricalReply); i++) Reply[i] = Serial.read();

    if (ConfirmChkSum(Reply, sizeof(TheoricalReply))) {
      returnThisStr(String(Reply[8], DEC));
    } else {
      returnThisStr("ERROR: incorrect stove response");
    }
  }
  else if (ReqType == "ScrewLoadingTime") { // Request loading time (s)
    uint8_t PreQuery[] = {0x21, 0x00, 0x10, 0x0b, 0x01, 0x2D, 0x5D};
    uint8_t TheoricalReply[] = {0x11, 0x00, 0x10, 0x0b, 0x01, 0xAA, 0x00, 0x00};
    uint8_t Reply[sizeof(TheoricalReply)];

    FixQuery_VarReply(PreQuery, sizeof(PreQuery));
    for (uint8_t i = 0; i < sizeof(PreQuery); i++) Serial.read();
    for (uint8_t i = 0; i < sizeof(TheoricalReply); i++) Reply[i] = Serial.read();

    if (ConfirmChkSum(Reply, sizeof(TheoricalReply))) {
      returnThisStr(String(Reply[8], DEC));
    } else {
      returnThisStr("ERROR: incorrect stove response");
    }
  }
  else if (ReqType == "ScrewLoadingRemaining") { // Request remaining loading time (s)
    uint8_t PreQuery[] = {0x21, 0x00, 0x10, 0x0a, 0x02, 0x2E, 0x0F};
    uint8_t TheoricalReply[] = {0x11, 0x00, 0x10, 0x0a, 0x02, 0x00, 0xAA, 0x00, 0x00};
    uint8_t Reply[sizeof(TheoricalReply)];

    FixQuery_VarReply(PreQuery, sizeof(PreQuery));
    for (uint8_t i = 0; i < sizeof(PreQuery); i++) Serial.read();
    for (uint8_t i = 0; i < sizeof(TheoricalReply); i++) Reply[i] = Serial.read();

    if (ConfirmChkSum(Reply, sizeof(TheoricalReply))) {
      returnThisStr(String(Reply[8], DEC));
    } else {
      returnThisStr("ERROR: incorrect stove response");
    }
  }
  else if (ReqType == "PartialCounter") { // Requestpartial counter (h)
    uint8_t PreQuery[] = {0x21, 0x00, 0x06, 0xD1, 0x30};
    uint8_t TheoricalReply[] = {0x11, 0x00, 0x06, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0x00, 0x00};
    uint8_t Reply[sizeof(TheoricalReply)];

    FixQuery_VarReply(PreQuery, sizeof(PreQuery));
    for (uint8_t i = 0; i < sizeof(PreQuery); i++) Serial.read();
    for (uint8_t i = 0; i < sizeof(TheoricalReply); i++) Reply[i] = Serial.read();

    if (ConfirmChkSum(Reply, sizeof(TheoricalReply))) {
      returnThisStr(String(((Reply[4] & 0xFF) << 8) + ((Reply[3] & 0xFF) << 0), DEC));
    } else {
      returnThisStr("ERROR: incorrect stove response");
    }
  }
  else if (ReqType == "TotalCounter") { // Request total counter (h)
    uint8_t PreQuery[] = {0x21, 0x00, 0x06, 0xD1, 0x30};
    uint8_t TheoricalReply[] = {0x11, 0x00, 0x06, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0x00, 0x00};
    uint8_t Reply[sizeof(TheoricalReply)];

    FixQuery_VarReply(PreQuery, sizeof(PreQuery));
    for (uint8_t i = 0; i < sizeof(PreQuery); i++) Serial.read();
    for (uint8_t i = 0; i < sizeof(TheoricalReply); i++) Reply[i] = Serial.read();

    if (ConfirmChkSum(Reply, sizeof(TheoricalReply))) {
      returnThisStr(String(((Reply[6] & 0xFF) << 8) + ((Reply[5] & 0xFF) << 0), DEC));
    } else {
      returnThisStr("ERROR: incorrect stove response");
    }
  }
  else if (ReqType == "StartupCounter") { // Request number of start-up
    uint8_t PreQuery[] = {0x21, 0x00, 0x06, 0xD1, 0x30};
    uint8_t TheoricalReply[] = {0x11, 0x00, 0x06, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0x00, 0x00};
    uint8_t Reply[sizeof(TheoricalReply)];

    FixQuery_VarReply(PreQuery, sizeof(PreQuery));
    for (uint8_t i = 0; i < sizeof(PreQuery); i++) Serial.read();
    for (uint8_t i = 0; i < sizeof(TheoricalReply); i++) Reply[i] = Serial.read();

    if (ConfirmChkSum(Reply, sizeof(TheoricalReply))) {
      returnThisStr(String(((Reply[8] & 0xFF) << 8) + ((Reply[7] & 0xFF) << 0), DEC));
    } else {
      returnThisStr("ERROR: incorrect stove response");
    }
  }
  else if (ReqType == "ExhaustTemperature") { // Request temperature of exhaust gases
    uint8_t PreQuery[] = {0x21, 0x00, 0x10, 0x1e, 0x02, 0xE1, 0xB8};
    uint8_t TheoricalReply[] = {0x11, 0x00, 0x10, 0x1e, 0x02, 0xAA, 0x00, 0x00};
    uint8_t Reply[sizeof(TheoricalReply)];

    FixQuery_VarReply(PreQuery, sizeof(PreQuery));
    for (uint8_t i = 0; i < sizeof(PreQuery); i++) Serial.read();
    for (uint8_t i = 0; i < sizeof(TheoricalReply); i++) Reply[i] = Serial.read();

    if (ConfirmChkSum(Reply, sizeof(TheoricalReply))) {
      returnThisStr(String(Reply[8], DEC));
    } else {
      returnThisStr("ERROR: incorrect stove response");
    }
  }
  else if (ReqType == "ElectronicTemperature") { // Request temperature of the electronic
    uint8_t PreQuery[] = {0x21, 0x00, 0x10, 0x12, 0x01, 0x94, 0xB6};
    uint8_t TheoricalReply[] = {0x11, 0x00, 0x10, 0x12, 0x01, 0xAA, 0x00, 0x00};
    uint8_t Reply[sizeof(TheoricalReply)];

    FixQuery_VarReply(PreQuery, sizeof(PreQuery));
    for (uint8_t i = 0; i < sizeof(PreQuery); i++) Serial.read();
    for (uint8_t i = 0; i < sizeof(TheoricalReply); i++) Reply[i] = Serial.read();

    if (ConfirmChkSum(Reply, sizeof(TheoricalReply))) {
      returnThisStr(String(Reply[8], DEC));
      // To be divided by 2
    } else {
      returnThisStr("ERROR: incorrect stove response");
    }
  } else
    returnThisStr("ERROR: request not recognized");
}

// Function to calculate checksum and return complete frame
uint8_t GetChkSum(uint8_t frame[], uint8_t length) {
  Crc16 crc;
  crc.clearCrc();
  uint16_t TtlChkSum;
  // Get checksum
  TtlChkSum = crc.XModemCrc(frame, 0, length - 2);
  // Complete frame
  frame[length - 1] = TtlChkSum >> 0;
  frame[length - 2] = TtlChkSum >> 8;

  return frame[length];
}

// Function to confirm the checksum of stove response
bool ConfirmChkSum(uint8_t frame[], uint8_t length) {
  uint8_t ChkSum1;
  uint8_t ChkSum2;
  Crc16 crc;
  crc.clearCrc();
  uint16_t TtlChkSum;
  // Get checksum
  TtlChkSum = crc.XModemCrc(frame, 0, length - 2);
  ChkSum2 = TtlChkSum >> 0;
  ChkSum1 = TtlChkSum >> 8;

  // returnThisStr(String(ChkSum1, HEX) + " == " + String(frame[length - 2], HEX));
  // returnThisStr(String(ChkSum2, HEX) + " == " + String(frame[length - 1], HEX));

  if (ChkSum1 == frame[length - 2] && ChkSum2 == frame[length - 1]) {
    return true;
  } else {
    return false;
  }
}

// Function to send a variable message and receive a known reply
void VarQuery_FixReply(uint8_t Query[], uint8_t QueryL, uint8_t TReply[], uint8_t TReplyL) {
  uint8_t Compt = 0;

  // Get checksum of request and reply
  Query[QueryL] = GetChkSum(Query, QueryL);
  TReply[TReplyL] = GetChkSum(TReply, TReplyL);

  // Purge remaining data on buffer
  while (Serial.available() > 0) Serial.read();

  // Send query to stove and wait for it's reply
  Serial.write(Query, QueryL);
  Serial.flush();
  delay(50);

  // Get reply from stove and compare to theoritical reply
  for (uint8_t i = 0; i < QueryL; i++) Serial.read();
  for (uint8_t i = 0; i < TReplyL; i++) {
    if (Serial.read() != TReply[i]) Compt++;
  }

  // Send back the status to python
  if (Compt == 0) {
    returnThisStr("ok");
  } else {
    returnThisStr("nok");
  }
}

//Function to send a known message and receive a variable reply
void FixQuery_VarReply(uint8_t Query[], uint8_t QueryL) {
  uint8_t Compt = 0;
  uint8_t Escape = 0;
  String ReplyData = "";

  // Purge remaining data on buffer
  while (Serial.available() > 0) Serial.read();

  // Send query to stove and wait for it's reply
  Serial.write(Query, QueryL);
  Serial.flush();
  delay(250);
}

// Function to send a known message and receive a known reply
void FixQuery_FixReply(uint8_t Query[], uint8_t QueryL, uint8_t TReply[], uint8_t TReplyL) {
  uint8_t ReceivedHexa[TReplyL];
  uint8_t Compt = 0;

  // Purge remaining data on buffer
  while (Serial.available() > 0) Serial.read();

  // Send query to stove and wait for it's reply
  Serial.write(Query, QueryL);
  Serial.flush();
  delay(50);

  // Get reply from stove and compare to theoritical reply
  for (uint8_t i = 0; i < QueryL; i++) Serial.read();
  for (uint8_t i = 0; i < TReplyL; i++) ReceivedHexa[i] = Serial.read();

  for (uint8_t i = 0; i < TReplyL; i++) {
    if (ReceivedHexa[i] != TReply[i]) Compt++;
  }

  if (Compt == 0) {
    returnThisStr("ok");
  } else {
    returnThisStr("nok");
  }
}