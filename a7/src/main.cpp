#include <Arduino.h>

//partly taken from IMST arduino library example

//wimod comm.
#include <HardwareSerial.h>
HardwareSerial loraSerial(2);
#define WIMOD_IF    loraSerial
#define WIMOD_IF_RX 23
#define WIMOD_IF_TX 05
#define PC_IF    Serial
uint8_t devEUI[8];

//wimod/LoRa join
#include <WiMODLoRaWAN.h> // make sure to use only the WiMODLoRaWAN.h, the WiMODLR_BASE.h must not be used for LoRaWAN firmware.
WiMODLoRaWAN wimod(WIMOD_IF);

const unsigned char APPEUI[] = { 0x70, 0xB3, 0xD5, 0x8F, 0xF1, 0x00, 0x4A, 0x2A };
const unsigned char APPKEY[] = { 0xE4, 0x3E, 0xFF, 0xF6, 0x56, 0x9D, 0xEA, 0xE2, 0x9C, 0x7D, 0xD2, 0xA0, 0x3F, 0x63, 0xEC, 0xC3 };

//LoRa app
boolean sendLora;
unsigned long lastSent = 0;
#define loraBytesSize 11
byte loraBytes[loraBytesSize];

// Typedefs
typedef enum TModemState {
    ModemState_Disconnected = 0,
    ModemState_ConnectRequestSent,
    ModemState_Connected,
    ModemState_FailedToConnect,
} TModemState;

typedef struct TRuntimeInfo {
    TModemState ModemState;
} TRuntimeInfo;

//section RAM: Create in instance of the interface to the WiMOD-LR-Base firmware
TRuntimeInfo RIB = {  };
static TWiMODLORAWAN_TX_Data txData; //contains UINT8 TWiMODLORAWAN_TX_Data.Payload[128]

//Helperfunctions
void debugMsg(String msg) { PC_IF.print(msg); }
void debugMsg(int a) { PC_IF.print(a, DEC); }
void debugMsgChar(char c) { PC_IF.print(c); }
void debugMsgHex(int a) {
    if (a < 0x10) {
        PC_IF.print(F("0"));
    }
    PC_IF.print(a, HEX);
}

void printPayload(const uint8_t* buf, uint8_t size) {
  for (int i = 0; i < size; i++) {
	if ((uint8_t) buf[i] < 0x10) {
		PC_IF.print(F("0"));
	}
	PC_IF.print((uint8_t) buf[i], HEX);
	PC_IF.print(F(" "));
  }
  PC_IF.print(F("\n"));
}

void print_lora_config() {
  TWiMODLORAWAN_RadioStackConfig radioCfgnew;  
  wimod.GetRadioStackConfig(&radioCfgnew);
  PC_IF.print("DataRateIndex: "); PC_IF.println(radioCfgnew.DataRateIndex);
  PC_IF.print("TXPowerLevel: "); PC_IF.println(radioCfgnew.TXPowerLevel);
  PC_IF.print("Options: "); PC_IF.println(radioCfgnew.Options);
  PC_IF.print("-> ADR: "); PC_IF.println(radioCfgnew.Options & LORAWAN_STK_OPTION_ADR);
  PC_IF.print("-> DUTYCYCLE: "); PC_IF.println(radioCfgnew.Options & LORAWAN_STK_OPTION_DUTY_CYCLE_CTRL);
  PC_IF.print("-> CLASSC: "); PC_IF.println(radioCfgnew.Options & LORAWAN_STK_OPTION_DEV_CLASS_C);
  PC_IF.print("-> POWERUPIND: "); PC_IF.println(radioCfgnew.Options & LORAWAN_STK_OPTION_POWER_UP_IND);
  PC_IF.print("-> PRIVATENETWORK: "); PC_IF.println(radioCfgnew.Options & LORAWAN_STK_OPTION_PRIVATE_NETOWRK);
  PC_IF.print("-> EXTPKT: "); PC_IF.println(radioCfgnew.Options & LORAWAN_STK_OPTION_EXT_PKT_FORMAT);
  PC_IF.print("-> MACCMD: ");  PC_IF.println(radioCfgnew.Options & LORAWAN_STK_OPTION_MAC_CMD);
  PC_IF.print("PowerSavingMode: "); PC_IF.println(radioCfgnew.PowerSavingMode);
  PC_IF.print("Retransmissions: "); PC_IF.println(radioCfgnew.Retransmissions);
  PC_IF.print("BandIndex: "); PC_IF.println(radioCfgnew.BandIndex);
}

void config_lora_radio() {
  /* options
   * #define LORAWAN_STK_OPTION_ADR                  (0x01 << 0=00000001)                     Stack option ADR
   * #define LORAWAN_STK_OPTION_DUTY_CYCLE_CTRL      (0x01 << 1=00000010)                     Stack option Duty Cycle Control
   * #define LORAWAN_STK_OPTION_DEV_CLASS_C          (0x01 << 2=00000100)                     Stack option ClassC device
   * #define LORAWAN_STK_OPTION_POWER_UP_IND         (0x01 << 4=00010000)                     Stack option for sending a PowerUp Indication
   * #define LORAWAN_STK_OPTION_PRIVATE_NETOWRK      (0x01 << 5=00100000)                     Stack option for using a Private LoRaWAN Network; 0 = public LoRaWAN network
   * #define LORAWAN_STK_OPTION_EXT_PKT_FORMAT       (0x01 << 6=01000000)                     Stack option extended format
   * #define LORAWAN_STK_OPTION_MAC_CMD              (0x01 << 7=10000000)                     Stack option forwad MAC commands
   */
  //print_lora_config();
  TWiMODLORAWAN_RadioStackConfig radioCfg;
  wimod.GetRadioStackConfig(&radioCfg);
  TWiMODLRResultCodes res;
  UINT8 sta;
  radioCfg.DataRateIndex = LoRaWAN_DataRate_EU868_LoRa_SF7_125kHz; //see TLoRaWANDataRate
  radioCfg.TXPowerLevel = 16; //from 0 to 20
  radioCfg.Options = LORAWAN_STK_OPTION_DUTY_CYCLE_CTRL; //| LORAWAN_STK_OPTION_ADR; //ADR off for non-stationary nodes=coverage mapping
  radioCfg.PowerSavingMode = 1;
  radioCfg.Retransmissions = 7; //max number of retransmissions (for C-Data) to use
  radioCfg.BandIndex = LORAWAN_BAND_EU_868_RX2_SF9; //SF9BW125 is used for RX2 by TTN; alternative: LORAWAN_BAND_EU_868
  //radioCfg.HeaderMacCmdCapacity = 15;
  // set new radio config
  Serial.print("Radio config successful, when 100 shown: "); Serial.print(wimod.SetRadioStackConfig(&radioCfg, &res, &sta)); Serial.print(res); Serial.println(sta);

  /* Direct HCI instead of wimod.SetRadioStackConfig also working:
   *  Format: DataRate (DR3=SF9 DR5=SF7 ...), TXpower, options see above, PowerSaving Mode (1=on), Retransmissions, BandIndex, HeaderMacCmdCap
  UINT8 sf7adron[] = {0x05, 0x10, 0x03, 0x01, 0x07, 0x01, 0x0F};
  UINT8 sf12adron[] = {0x00, 0x10, 0x03, 0x01, 0x07, 0x01, 0x0F};
  wimod.SendHCIMessage((UINT8) 0x10, (UINT8) 0x19, (UINT8) 0x1A, sf12adron, (UINT16) 7); //dstSapID, msgID, rxMsgID, payload, length
  //PAYLOAD               CRC16
  //05 10 03 01 07 01 0F   46 67 //SF7, ADR on
  //05 10 02 01 07 01 0F   02 6C //SF7, ADR off
  //00 10 02 01 07 01 0F   A1 9C //SF12, ADR off
  //00 10 03 01 07 01 0F   E5 97 //SF12, ADR on*/
  //optional: wimod.Process();
  delay(500);
  print_lora_config();
}

//join tx indication callback
void onJoinTx(TWiMODLR_HCIMessage& rxMsg) {
    TWiMODLORAWAN_TxIndData txData;
    wimod.convert(rxMsg, &txData);
    debugMsg(F("joining attempt: "));
    debugMsg((int) txData.NumTxPackets);
    debugMsg(F("\n"));
}

//joined network indication
void onJoinedNwk(TWiMODLR_HCIMessage& rxMsg) {
    TWiMODLORAWAN_RX_JoinedNwkData joinedData;

    debugMsg(F("Join-Indication received.\n"));

    if (wimod.convert(rxMsg, &joinedData)) {
        if ((LORAWAN_JOIN_NWK_IND_FORMAT_STATUS_JOIN_OK == joinedData.StatusFormat)
                || (LORAWAN_JOIN_NWK_IND_FORMAT_STATUS_JOIN_OK_CH_INFO == joinedData.StatusFormat)){
            //Ok device is now joined to nwk (server)
            RIB.ModemState = ModemState_Connected;

            debugMsg(F("Device has joined a network.\n"));
            debugMsg(F("New Device address is: "));
            debugMsg((int) joinedData.DeviceAddress);
            debugMsg(F("\n"));
        } else {
            // error joining procedure did not succeed
            RIB.ModemState = ModemState_FailedToConnect;
            debugMsg(F("Failed to join a network.\n"));
        }
    }
}

//rx data callback
void onRxData(TWiMODLR_HCIMessage& rxMsg) {
  debugMsg("Rx-Data Indication received.\n");
  TWiMODLORAWAN_RX_Data radioRxMsg;
  int i;

  // convert/copy the raw message to RX radio buffer
  if (wimod.convert(rxMsg, &radioRxMsg)) {

  if (radioRxMsg.StatusFormat & LORAWAN_FORMAT_ACK_RECEIVED) { // this is an ack
    debugMsg(F("Ack-Packet received."));
  }
      // print out the received message as hex string
      if (radioRxMsg.Length > 0) {
          // print out the length
          debugMsg(F("Rx-Message: ["));
          debugMsg(radioRxMsg.Length);
          debugMsg(F("]: "));

          // print out the payload
          for (i = 0; i < radioRxMsg.Length; i++) {
              debugMsgHex(radioRxMsg.Payload[i]);
              debugMsg(F(" "));
          }
          debugMsg(F("\n"));
      } else {  // no payload included
        debugMsg(F("\n")); //debugMsg(F("Rx-Message with no Payload received; Status: ")); debugMsg((int) radioRxMsg.StatusFormat);
      }
  }
}


void setup()
{
  //debug
  pinMode(BUILTIN_LED, OUTPUT);
  PC_IF.begin(115200);

  //LoRa
  WIMOD_IF.begin(WIMOD_LORAWAN_SERIAL_BAUDRATE, SERIAL_8N1, WIMOD_IF_RX, WIMOD_IF_TX); //rx tx
  wimod.begin(); // init the communication stack
  delay(100); wimod.Reset(); delay(100); // do a software reset of the WiMOD
  wimod.DeactivateDevice(); // deactivate device in order to get a clean start


    if (wimod.GetDeviceEUI(devEUI)) {
    	debugMsg(F("\r\n"));
    	debugMsg(F("LoRaWAN Info:\r\n"));
		  debugMsg(F("-------------\r\n"));
    	debugMsg(F("DeviceEUI:       "));
    	printPayload(devEUI, 0x08);
    }

  // do a simple ping to check the local serial connection
  debugMsg(F("Ping WiMOD: "));
  if (wimod.Ping() != true) {
      debugMsg(F("FAILED\n"));
  } else {
      debugMsg(F("OK. Starting join OTAA procedure...\n"));
      config_lora_radio();
  
      //setup OTAA parameters
      TWiMODLORAWAN_JoinParams joinParams;
      memcpy(joinParams.AppEUI, APPEUI, 8);
      memcpy(joinParams.AppKey, APPKEY, 16);
      wimod.SetJoinParameter(joinParams);
  
      // Register callbacks for join related events
      wimod.RegisterJoinedNwkIndicationClient(onJoinedNwk);
      wimod.RegisterJoinTxIndicationClient(onJoinTx);
      wimod.RegisterRxUDataIndicationClient(onRxData);
      //wimod.RegisterRxCDataIndicationClient(onRxData);
      //wimod.RegisterRxAckIndicationClient(onRxData);
      //wimod.RegisterRxMacCmdIndicationClient(onRxData);
  
      // send join request
      if (wimod.JoinNetwork()) {
        RIB.ModemState = ModemState_ConnectRequestSent;
        debugMsg(F("...waiting for nwk response...\n"));
      } else {
        debugMsg("Error sending join request: ");
        debugMsg((int) wimod.GetLastResponseStatus());
        debugMsg(F("\n"));
      }
  }
}

void loop()
{
  sendLora = true;
  if(millis() - lastSent < 15000) sendLora = false; //send every 15 sec
  if(RIB.ModemState != ModemState_Connected) sendLora = false; // check of OTAA procedure has finished
  if(sendLora) digitalWrite(BUILTIN_LED, HIGH); else digitalWrite(BUILTIN_LED, LOW);

  if(sendLora) {
    debugMsg(F("Sending...\n"));

    // prepare TX data structure for string
    txData.Port = 0x01;
    txData.Length = loraBytesSize;
	strcpy_P((char*) txData.Payload, PSTR("Hello World"));

	// prepare TX data structure for bytes
    //txData.Port = 0x02;
    //txData.Length = loraBytesSize;
    //memcpy(txData.Payload, loraBytes, txData.Length);
  
    // try to send a message
    if (false == wimod.SendUData(&txData)) { // an error occurred
         if (LORAWAN_STATUS_CHANNEL_BLOCKED == wimod.GetLastResponseStatus()) {// we have got a duty cycle problem
             debugMsg(F("TX failed: Blocked due to DutyCycle...\n"));
         }
    } else {
      lastSent = millis();
    }
  }
  // check for any pending data of the WiMOD
  wimod.Process();

  delay(500);
}