#include "BigInteger.h"
#include "FS.h"
#include <SPI.h>
#include <TFT_eSPI.h> 
#include "WiFi.h"
#include <PubSubClient.h>

#define CALIBRATION_FILE "/TouchCalData3"
#define REPEAT_CAL false
#define K 75 //resonable value

const char PASSWORD[] = "Capparelli96"; //same password for all networks
const char mqttBROKER[] = "192.168.1.201"; //ip of raspberry .201
const char mqttCLIENT[] = "player1";

volatile uint32_t N, O, d, c1, c2, privateKey[2], publicKey_1[2], publicKey_2[2];
volatile uint8_t p, q, e, c, crivello_set[K], caso, nNet;
volatile boolean flag = false, turn, rsa = false, sent = false, received = false, b1 = false;
volatile char segno = '-', matrix[3][3];

String ssidNet[2], msgIn;

WiFiClient espClient; 
PubSubClient client(espClient);

TFT_eSPI tft = TFT_eSPI();

uint8_t contain(volatile uint8_t* vet, uint32_t n, uint8_t elem){
  for(uint32_t i = 0; i < n; i++)
     if(vet[i] == (elem))
      return 1;
  return 0;
}

uint8_t* crivello(){
  uint8_t* primes = (uint8_t*) malloc(1 * sizeof(uint8_t));;
  
  for ( uint8_t i = 0; i <= K; i++ ) 
        crivello_set[i] = i;

  c = 0;
  for( uint8_t  i=2; i<16; i = (i==2)? i+1 : i+2)
    if (contain(crivello_set, K, i))
        for (uint8_t j=2*i; j <= K; j+=i)
              crivello_set[j] = 0;

   c = 0; 
   for ( uint8_t i = 0; i < K; ++i) 
     if(crivello_set[i] != 0) {
      c++;
      primes = (primes != 0)?(uint8_t*) realloc(primes, c * sizeof(uint8_t)):primes = (uint8_t*) malloc(c * sizeof(uint8_t));
      primes[c-1] = crivello_set[i];
      
     }
     
   return primes;
}

void estrai(){
  
  uint8_t* primes = crivello();
  
  p = primes[uint8_t(random(5, c-1))];
  q = primes[uint8_t(random(10, c-1))];

  while ( q == p ) q = primes[uint8_t(random(10, c))];
  
  free(primes);
  
  N = p*q;
  O = (p-1)*(q-1);

  e = 1;
  c = 0;
  for (uint32_t i = 2; i <= O; i++){
       uint32_t* divO = divisori(O, i); 
       uint32_t* divE = divisori(i, i);

       for(uint32_t k = 0; k<c1; k ++)
           for(uint32_t j = 0; j<c2; j ++)
               if(divO[k] == divE[j])
                  c = divO[k];
       if (c == 1) {
           e = i;
           free(divO);
           free(divE);
           break;
       }                       
  }
  
  d = 0;
  for (uint32_t i = 2; i <= O; i++){
    d = i;
    if (((d*e)%O) == 1) break;
  }
}

String publicKeyToString(){
  return String(e) + "," + String(N);
}
String publicKey_2_ToString(){
  return String(publicKey_2[0]) + "," + String(publicKey_2[1]);
}

String privateKeyToString(){
  return String(d) + "," + String(N);
}

void resetKeys(){
   publicKey_2[0] = 0;
   publicKey_2[1] = 0;
   
   privateKey[0] = 0; privateKey[1] = 0;
   publicKey_1[0] = 0; publicKey_1[1] = 0;
}

uint32_t*  divisori (uint32_t a, uint32_t limit){
    uint32_t* divis = 0;
    uint32_t cn = 1;
    for(uint8_t i = 1; i<=limit; i++)
        if( a%i == 0 ) {
            divis = (divis != 0)?(uint32_t*) realloc(divis, cn * sizeof(uint32_t)):divis = (uint32_t*) malloc(cn * sizeof(uint32_t));
            divis[cn-1]=i;
            cn++;
        }

     if(a == O) c1 = cn-1;
     else c2 = cn-1;
     
     return divis;
}

String encrypte(uint8_t m){
  
    BigInteger N1 = BigInteger(publicKey_2[1]);
    BigInteger e1 = BigInteger(publicKey_2[0]);
    
    BigInteger m1 = BigInteger(m);

    BigInteger enc = (m1^e1)%N1;
    
    return convert(enc);
}


String decrypte(String enc){
    BigInteger N1 = BigInteger(privateKey[1]);
    BigInteger d1 = BigInteger(privateKey[0]);
    
    BigInteger enc1 = BigInteger(enc.toInt());
    
    BigInteger dec = (enc1^d1)%N1;
    
    return convert(dec);
}

string convert2(String m){
  string s = "";
  for (int i = 0; i < m.length(); i++)
    s[i] = m[i]; 
  return s;
}


String convert(BigInteger b){
  String s = "";
  for (int i = 0; i < b.stringTo().length(); i++)
    s.concat(b.stringTo()[i]); 
  return s;
}
  

void setup() {
  delay(5000);
  
  caso = 0;
  
  tft.init();
  tft.setRotation(3); //horizontal
  touch_calibrate();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  resetKeys();
  emptyMatrix();
  defaultScreen();
}

void loop() {
  uint16_t x, y;
  if (tft.getTouch(&x, &y))
       eventScreen(caso, x, y);
  eventScreen(caso, -1, -1);
}

void defaultScreen(){
  tft.fillScreen(TFT_ORANGE);

  setText("Tic Tac Toe", 155, 0, 3, TFT_OLIVE);
  setBtn("Scan WiFi", 90, 50, 120, 40, 1, TFT_DARKGREY, TFT_WHITE, TFT_WHITE);
  setBtn("About Project", 90, 100, 120, 40, 1, TFT_DARKGREY, TFT_WHITE, TFT_WHITE);

  setText(" IOT SECURITY PROJECT - Fabio Capparelli -  214490", 0, 230, 1, TFT_OLIVE);
}


void aboutProjectScreen(){
    tft.fillScreen(TFT_ORANGE);
    setBtn("<-", 1, 1, 20, 20, 1, TFT_DARKGREY, TFT_WHITE, TFT_WHITE);
    setText("About Project", 170, 0, 3, TFT_OLIVE);
    setText("The following has as main aim to develop a minigame", 1, 60, 1, TFT_OLIVE);
    setText("TIC TAC TOE 1vs1, by giving the possibility to choose", 1, 80, 1, TFT_OLIVE);
    setText("the comunication protocol.", 1, 100, 1, TFT_OLIVE);
    setText("You can choose MQTT, and the modality", 1, 120, 1, TFT_OLIVE);
    setText("in which you can encrypte your messages, ", 1, 140, 1, TFT_OLIVE);
    setText("using asymmetric RSA algorithm or not.", 1, 160, 1, TFT_OLIVE);
}

void wifiScreen(){
  tft.fillScreen(TFT_ORANGE);
  setBtn("<-", 1, 1, 20, 20, 1, TFT_DARKGREY, TFT_WHITE, TFT_WHITE);
  setText("Connect to WiFi", 170, 0, 3, TFT_OLIVE);
  
  setText("Scanning      ", 170, 40, 2, TFT_OLIVE);
  delay(200);
  setText("Scanning .    ", 170, 40, 2, TFT_OLIVE);
  delay(200);
  setText("Scanning . .  ", 170, 40, 2, TFT_OLIVE);
  delay(200);
  setText("Scanning . . .", 170, 40, 2, TFT_OLIVE);
  delay(200);
  setText("         . . .", 170, 40, 2, TFT_ORANGE);
  delay(200);
  setText("Scanning      ", 170, 40, 2, TFT_OLIVE);
  delay(200);
  setText("Scanning .    ", 170, 40, 2, TFT_OLIVE);
  delay(200);
  setText("Scanning . .  ", 170, 40, 2, TFT_OLIVE);
  delay(200);
  setText("Scanning . . .", 170, 40, 2, TFT_OLIVE);
  delay(200);

  uint8_t nNetworks = WiFi.scanNetworks();
  if (nNetworks == 0) 
  {
    setText("Scanning . . .", 170, 40, 2, TFT_ORANGE);
    setText("no networks found", 170, 40, 1, TFT_OLIVE);
  }
  else 
  {
    setText("Scanning . . .", 170, 40, 2, TFT_ORANGE);
    setText(" " + String(nNetworks) + " networks found", 170, 40, 1, TFT_OLIVE);
    setText(" YOU CAN CONNECT ONLY TO fastweb_Capparelli NETWORK", 0, 230, 1, TFT_OLIVE);
   
    c=0;
    for (int i = 0; i < nNetworks; ++i) 
    {
      String ssid = WiFi.SSID(i);
      if(ssid.startsWith("fastweb_Capparelli 2"))
      {
        ssidNet[c] = ssid; 
        setText(String(c+1) + ": " + ssid, 0, (c+3)*20, 1, TFT_OLIVE);
        setBtn("Connect", 230, (c+3)*18, 80, 13, 1, TFT_DARKGREY, TFT_WHITE, TFT_WHITE);
        c++;
      }
    }
    nNet = c;
    
    for (int i = 0; i < nNetworks; ++i) 
    {
      String ssid = WiFi.SSID(i);
      if(!ssid.startsWith("fastweb_Capparelli 2"))
      {
        setText(String(c+1) + ": " + ssid, 0, (c+3)*20, 1, TFT_OLIVE);
        c++;
      }
    }
    
  }  
}

void connectingScreen(String ssid, String password){
  tft.fillScreen(TFT_ORANGE);

  setText(" SSID: " + ssid, 0, 70, 1, TFT_OLIVE);
  setText(" PASSWORD: " + password, 0, 100, 1, TFT_OLIVE);

  setText("Connecting      ", 170, 0, 3, TFT_OLIVE);
  delay(200);
  setText("Connecting .    ", 170, 0, 3, TFT_OLIVE);
  delay(200);
  setText("Connecting . .  ", 170, 0, 3, TFT_OLIVE);
  delay(200);
  setText("Connecting . . .", 170, 0, 3, TFT_OLIVE);
  delay(200);
  setText("           . . .", 170, 0, 3, TFT_ORANGE);
  delay(200);
  setText("Connecting      ", 170, 0, 3, TFT_OLIVE);
  delay(200);
  setText("Connecting .    ", 170, 0, 3, TFT_OLIVE);
  delay(200);
  setText("Connecting . .  ", 170, 0, 3, TFT_OLIVE);
  delay(200);
  setText("Connecting . . .", 170, 0, 3, TFT_OLIVE);
  
  delay(1000);
}

void connectedScreen(String ssid, String password, String ip, String gip, String sm){
  tft.fillScreen(TFT_ORANGE);

  setBtn("<-", 1, 1, 20, 20, 1, TFT_DARKGREY, TFT_WHITE, TFT_WHITE);
  setBtn("->", 230, 180, 60, 40, 2, TFT_DARKGREY, TFT_WHITE, TFT_WHITE);

  setText("CONNECTED", 170, 0, 3, TFT_OLIVE);
  setText(" SSID: " + ssid, 0, 70, 1, TFT_OLIVE);
  setText(" PASSWORD: " + password, 0, 100, 1, TFT_OLIVE);
  setText(" local_IP: " + ip, 0, 130, 1, TFT_OLIVE);  
  setText(" gateway_IP: " + gip, 0, 160, 1, TFT_OLIVE); 
  setText(" subnet_MASK: " + sm, 0, 190, 1, TFT_OLIVE); 
}

void protocolScreen(){
  tft.fillScreen(TFT_ORANGE);

  setText("Tic Tac Toe", 170, 0, 3, TFT_OLIVE);
  setText("choose the protocol: ", 170, 50, 2, TFT_OLIVE);
  
  setBtn("<-", 1, 1, 20, 20, 1, TFT_DARKGREY, TFT_WHITE, TFT_WHITE);
  
  setBtn("MQTT", 90, 80, 120, 40, 1, TFT_DARKGREY, TFT_WHITE, TFT_WHITE);
  
}

void mqttScreen(){
  tft.fillScreen(TFT_ORANGE);

  setText("Tic Tac Toe", 170, 0, 3, TFT_OLIVE);
  setText("MQTT protocol: ", 170, 50, 2, TFT_OLIVE);
  
  setBtn("<-", 1, 1, 20, 20, 1, TFT_DARKGREY, TFT_WHITE, TFT_WHITE);
  
  setBtn("Connect to Broker", 90, 100, 140, 40, 1, TFT_DARKGREY, TFT_WHITE, TFT_WHITE);

  
}

void rsaScreen(){
  tft.fillScreen(TFT_ORANGE);

  setText("Tic Tac Toe", 170, 0, 3, TFT_OLIVE);
  setText("select your choice: ", 170, 50, 2, TFT_OLIVE);
  
  setBtn("<-", 1, 1, 20, 20, 1, TFT_DARKGREY, TFT_WHITE, TFT_WHITE);
  
  setBtn("use RSA", 90, 80, 120, 40, 1, TFT_DARKGREY, TFT_WHITE, TFT_WHITE);
  setBtn("don't use", 90, 130, 120, 40, 1, TFT_DARKGREY, TFT_WHITE, TFT_WHITE);
}

void keyScreen(){
  tft.fillScreen(TFT_ORANGE);

  setText("Tic Tac Toe", 170, 0, 3, TFT_OLIVE);
  setBtn("<-", 1, 1, 20, 20, 1, TFT_DARKGREY, TFT_WHITE, TFT_WHITE);

  setText("your keys are: ", 170, 50, 2, TFT_OLIVE);
  setText("Private: (d,N)=(" + String(d) + ", " + String(N) + ")", 0, 80, 2, TFT_OLIVE);
  setText("Public : (e,N)=(" + String(e) + ", " + String(N) + ")", 0, 105, 2, TFT_OLIVE);

  setBtn("SEND KEY", 90, 130, 120, 30, 1, TFT_DARKGREY, TFT_WHITE, TFT_WHITE);
  
}

void signScreen(){
  tft.fillScreen(TFT_BLACK);
  setText("Tic Tac Toe", 170, 0, 3, TFT_WHITE);
  setBtn("<-", 1, 1, 20, 20, 1, TFT_ORANGE, TFT_BLACK, TFT_WHITE);
  
   //MQTT
   if(segno == '-'){
      setText("choose O or X", 170, 50, 2, TFT_WHITE);
      setBtn("O", 120, 100, 50, 40, 3, TFT_ORANGE, TFT_BLACK, TFT_WHITE);
      setBtn("X", 170, 100, 50, 40, 3, TFT_ORANGE, TFT_BLACK, TFT_WHITE);
   }     
}

void gameScreen(){
  tft.fillScreen(TFT_BLACK);
  setText("Tic Tac Toe", 170, 0, 2, TFT_WHITE);
  setText("you are: " + String(segno), 170, 30, 2, TFT_WHITE);
  setBtn("<-", 1, 1, 20, 20, 1, TFT_ORANGE, TFT_BLACK, TFT_WHITE);

  for(int i=0; i<3;i++)
    for(int j=0; j<3;j++)
         setBtn("", (i*50)+95, (j*50)+60, 50, 50, 3, TFT_ORANGE, TFT_BLACK, TFT_WHITE);
}

void winScreen(String s){
  delay(500);
  tft.fillScreen(TFT_BLACK);
  setText("Tic Tac Toe", 170, 0, 3, TFT_WHITE);
  setText(s + " (" + String(segno) + ")", 170, 80, 3, TFT_WHITE);
  setBtn("PLAY", 110, 100, 100, 50, 2, TFT_ORANGE, TFT_BLACK, TFT_WHITE);
}

void eventScreen(uint8_t i, uint16_t x, uint16_t y){
  
  char tpc1[] = "mossa ";  //Own Topic
  char tpc2[] = "mossa ";  //Opponent Topic
  
  switch(i){
//HOME Screen_______________________________________________________________________________________________________________
     case 0: 
        if( (x<=210 && x>=90) && (y<=90 && y>=50) ){
            wifiScreen(); 
            caso = 1;
        }
        if( (x<=210 && x>=90) && (y<=140 && y>=100) ){
            aboutProjectScreen(); 
            caso = 10;
        }
        break; //CASE 0

     case 10: 
        if( (x<=20 && x>=0) && (y<=20 && y>=0) ){
            defaultScreen(); 
            caso = 0;
          }
          break; //CASE 10 about
//WIFI Screen_______________________________________________________________________________________________________________         
     case 1: 
        if( (x<=20 && x>=0) && (y<=20 && y>=0) ){
            defaultScreen(); 
            caso = 0;
        }//IF
        
        for (int i = 0; i<nNet; i++){
            if( (x<=310 && x>=230) && (y<=((i+3)*18)+13 && y>=(i+3)*18) ){

              if(WiFi.status() == WL_CONNECTED) WiFi.disconnect();
              char ssid[ssidNet[i].length()+1];
              ssidNet[i].toCharArray(ssid, ssidNet[i].length()+1);
              WiFi.begin(ssid, PASSWORD);
              while (WiFi.status() != WL_CONNECTED){
                delay(500);
                connectingScreen(String(ssid), String(PASSWORD));
              }//WHILE
              connectedScreen(String(ssid), String(PASSWORD), WiFi.localIP().toString(), WiFi.gatewayIP().toString(), WiFi.subnetMask().toString());
              caso = 2;
          }//IF
        }//FOR
        break;// CASE 1
        
//CONNECTED Screen_______________________________________________________________________________________________________________
      case 2: 
        if( (x<=20 && x>=0) && (y<=20 && y>=0) ){
            wifiScreen();
            WiFi.disconnect();
            caso = 1;
        }//IF
        if( (x<=290 && x>=230) && (y<=220 && y>=180) ){
             protocolScreen();
             caso = 3;
        }//IF
        break;//CASE 2
        
//PROTOCOL Screen_______________________________________________________________________________________________________________        
      case 3: 
        if( (x<=20 && x>=0) && (y<=20 && y>=0) ){
            connectedScreen(WiFi.SSID(), String(PASSWORD), WiFi.localIP().toString(), WiFi.gatewayIP().toString(), WiFi.subnetMask().toString()); 
            caso = 2;
        }//IF
        
        if( (x<=210 && x>=90) && (y<=120 && y>=80)){          
            mqttScreen();
            caso = 4;
        }//IF
        break; //CASE 3
        
//MQTT SCREEN______________________________________________________________________________________________________________________        
      case 4: 
        if( (x<=20 && x>=0) && (y<=20 && y>=0) ){
            protocolScreen();
            caso = 3;
            client.disconnect();
        }//IF
        
         if( (x<=230 && x>=90) && (y<=140 && y>=100) ){
            client.setServer(mqttBROKER, 1883);
            client.connect(mqttCLIENT);
            client.setCallback(callback);
            
            if(client.connected()){
                setBtn("Connected", 90, 100, 140, 40, 1, TFT_GREEN, TFT_BLACK, TFT_WHITE);
                setBtn("NEXT", 110, 150, 100, 30, 1, TFT_DARKGREY, TFT_WHITE, TFT_WHITE);
                flag = true;
                caso = 4;
            }//IF
            else {
                setBtn("Not Connected", 90, 100, 140, 40, 1, TFT_RED, TFT_BLACK, TFT_WHITE);
                setBtn("RETRY", 110, 150, 100, 30, 1, TFT_DARKGREY, TFT_WHITE, TFT_WHITE);
                caso = 4;
            }//ESLE
        }//IF
        
        if(flag == true && ((x<=210 && x>=110) && (y<=180 && y>=150)) ){ //when click on next
              caso = 5;
              rsaScreen();
              flag = false;
        }//IF 
        
       break; //CASE 4
        
//RSA SCREEN_______________________________________________________________________________________________________________________
      case 5:
       if( (x<=20 && x>=0) && (y<=20 && y>=0) ){
            protocolScreen();
            caso = 3;
            if (client.connected()) client.disconnect();
        }//IF
        
        if(x<=210 && x>=90){
          
          if (y<=120 && y>=80) {
            rsa = true;
            estrai();
            resetKeys();
            keyScreen();
            caso = 6;
          }//IF
          if (y<=170 && y>=130) {
            rsa = false;
            signScreen();
            caso = 7;
          }//IF
          
        }//IF
        break; //CASE 5

//KEY SCREEN_______________________________________________________________________________________________________________________
      case 6: 
       if( (x<=20 && x>=0) && (y<=20 && y>=0) ){
            rsaScreen();
            caso = 5;
            rsa = false;
            resetKeys();
            if (client.connected()) client.disconnect();
       }//IF
       
       
        //MQTT_____________________________________________________________________________________________________________________
            
                if (!client.connected()) reconnectMqtt("key2");
                else client.subscribe("key2");
                client.loop(); 

                 if( (x<=210 && x>=90) && (y<=180 && y>=140) ){
                     privateKey[0] = d; privateKey[1] = N;
                     publicKey_1[0] = e; publicKey_1[1] = N;
                     String msgS = publicKeyToString();
                     char msg[msgS.length()+1];
                     msgS.toCharArray(msg, msgS.length()+1); 
                     publishMessage("key1", msg);
                     sent = true;
                     caso = 6;
                  }//IF
                  
             
                if(publicKey_2[0] == 0 && publicKey_2[1] == 0 && msgIn != "") {
                    uint8_t v=0;
                    for (uint8_t i = 0; i<msgIn.length(); i++)
                      if(msgIn[i] == ',') v = i;
                    
                    publicKey_2[0] = msgIn.substring(0, v).toInt();
                    publicKey_2[1] = msgIn.substring(v+1, msgIn.length()).toInt();
                    received = true;
                    setText("Received: (e,N)=(" + String(publicKey_2[0]) + ", " + String(publicKey_2[1]) + ")", 0, 180, 2, TFT_OLIVE);
                    msgIn = "";
                    caso = 6;    
                }//IF
              
              
              if(sent && received) {
                setBtn("PLAY", 90, 200, 120, 30, 1, TFT_DARKGREY, TFT_WHITE, TFT_WHITE);
                b1 = true;
                sent = false;
                received = false;
              }//IF
              
              if ( b1 && (x<=210 && x>=90) && (y<=230 && y>=200)){
                  client.unsubscribe("key1");
                  client.disconnect();
                  signScreen();
                  msgIn = "";
                  b1 = false;
                  caso = 7;
                }//IF
                
       break; // CASE 6
              
//SIGN SCREEN______________________________________________________________________________________________________________________ 
      case 7: //SIGN SCREEN
       if( (x<=20 && x>=0) && (y<=20 && y>=0) ){
            protocolScreen();
            caso = 3;
            segno = '-';
            rsa = false;
            if (client.connected()) client.disconnect();
           
        }//IF
        
       
          //MQTT___________________________________________________________________
           
            if (!client.connected()) reconnectMqtt("segno");
            else client.subscribe("segno");
            client.loop(); 
            
            if(segno == '-' && msgIn == "O") {
              segno = 'X';
              msgIn = "";
              caso = 8;
              turn = false; //opponent turn
              client.unsubscribe("segno");
              gameScreen();
              client.disconnect();
            }//IF
            
            if(segno == '-' && msgIn == "X" ) {
              segno = 'O';
              msgIn = "";
              caso = 8;
              turn = false; //opponent turn
              client.unsubscribe("segno");
              gameScreen();
              client.disconnect();
            }//IF

            if(segno == '-' && msgIn != "" && rsa){
              Serial.println("msgIn: " + msgIn);
              msgIn = char(decrypte(msgIn).toInt());
            }//IF
            
            if(segno == '-' && (x<=170 && x>=120) && (y<=140 && y>=100)){
              
              if(rsa) {
                String encS = encrypte('O');
                //Serial.println("segno: " + encS);
                char encC[encS.length()+1];
                encS.toCharArray(encC, encS.length()+1);
                publishMessage("segno", encC);
              }//IF
              else publishMessage("segno", "O"); //ELSE
              
              msgIn="";
              segno = 'O';
              caso = 8;
              turn = true; //my turn
              gameScreen();
              client.unsubscribe("segno");
              client.disconnect();
            }//IF
            
            if(segno == '-' && (x<=220 && x>=170) && (y<=140 && y>=100)){
             
              if(rsa) {
                String encS = encrypte('X');
                char encC[encS.length()+1];
                encS.toCharArray(encC, encS.length()+1);
                publishMessage("segno", encC);
              }//IF
              else publishMessage("segno", "X"); //ELSE
              
              msgIn="";
              segno = 'X';
              caso = 8;
              turn = true; //my turn
              gameScreen();
              client.unsubscribe("segno");
              client.disconnect();
            }//IF
     
      break; //CASE 7
//GAME SCREEN______________________________________________________________________________________________________
      case 8: 
       if( (x<=20 && x>=0) && (y<=20 && y>=0) ){
            protocolScreen();
            caso = 3;
            segno = '-';
            msgIn="";
            emptyMatrix();
            if (client.connected()) client.disconnect();
            
        }//IF
        
        //MQTT___________________________________________________________________________
            

            tpc1[5] = segno;
            tpc2[5] = (segno=='O')?'X':'O';

            if (!client.connected()) reconnectMqtt(tpc2);//mi sottoscrivo al topic avversario
            else subscribeMessage(tpc2);
            client.loop();

            opponentMoves();

            if(turn)
              for(int i=0; i<3; ++i)
                for(int j=0; j<3; ++j)
                  if( ((matrix[i][j] != 'X') && (matrix[i][j] != 'O')) && ( x<=((j+1)*50)+95 && x>=(j*50)+95 && y<=((i+1)*50)+60 && y>=(i*50)+60 ) ){
                      String msgS = String(i+1) + String(j+1);
                      if(rsa){
                        msgS = encrypte(msgS.toInt());
                        char msg[msgS.length()+1];
                        msgS.toCharArray(msg, msgS.length()+1); 
                        publishMessage(tpc1, msg);
                      }else{
                        char msg[msgS.length()+1];
                        msgS.toCharArray(msg, msgS.length()+1); 
                        publishMessage(tpc1, msg);
                      }
                      
                      matrix[i][j] = segno;
                      setBtn(String(segno), (j*50)+95, (i*50)+60, 50, 50, 3, TFT_ORANGE, TFT_BLACK, TFT_WHITE);
                      turn = false;
                      if(win()) {
                         winScreen("You Won");
                         caso = 9;
                      }//IF
                      if(parity()){
                         winScreen("No one won");
                         caso = 9;
                      }//IF
                  }//IF

          
        break; //  CASE 8
        
       //WIN SCREEN_________________________________________________________________________________________________
       case 9:
           if( (x<=210 && x>=110) && (y<=150 && y>=100) ){
            protocolScreen();
            caso = 3;
            segno = '-';
            resetKeys();
            emptyMatrix();
            if (client.connected()) client.disconnect();
            
        }//IF
        break; //CASE 9
       
     }//SWITCH CASES SCREEN
  
}//EVENT SCREEN

//OPPONENT MOVES_______________________________________________________________________________________________________
void opponentMoves(){
  
   uint8_t r,c;  //row column sign, y=row , x=column
   
   if(!turn)
      if(msgIn != ""){
         if(rsa) msgIn = decrypte(msgIn);
       
         r = uint8_t(msgIn[0])-49;
         c = uint8_t(msgIn[1])-49;
  
         if(matrix[r][c] != 'X' && (matrix[r][c] != 'O') && (matrix[r][c] == '-')){
            matrix[r][c] = (segno=='O')?'X':'O';
            setBtn(String(matrix[r][c]), (c*50)+95, (r*50)+60, 50, 50, 3, TFT_ORANGE, TFT_BLACK, TFT_WHITE);
            turn = true;
            if(win()) {
               winScreen("You Lost");
               caso = 9;
            }
            if(parity()){
               winScreen("No one won");
               caso = 9;
            }
         }
         msgIn="";
   }
}


void setText(String text, int x, int y, int s,  int color){
  tft.setTextColor(color);
  tft.setTextSize(s);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(text, x, y);
}


void setBtn(String label, int x, int y, int w, int h, int s, int fillColor, int labelColor, int cornerColor){
  tft.drawRect(x-1, y-1, w+2, h+2, cornerColor);
  tft.fillRect(x, y, w, h, fillColor);
  tft.setTextColor(labelColor);
  tft.setTextSize(s);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(label, x + (w/2), y + (h / 2));
}

void touch_calibrate()
{
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // check file system exists
  if (!SPIFFS.begin()) {
    Serial.println("Formating file system");
    SPIFFS.format();
    SPIFFS.begin();
  }

  // check if calibration file exists and size is correct
  if (SPIFFS.exists(CALIBRATION_FILE)) {
    if (REPEAT_CAL)
    {
      // Delete if we want to re-calibrate
      SPIFFS.remove(CALIBRATION_FILE);
    }
    else
    {
      File f = SPIFFS.open(CALIBRATION_FILE, "r");
      if (f) {
        if (f.readBytes((char *)calData, 14) == 14)
          calDataOK = 1;
        f.close();
      }
    }
  }

  if (calDataOK && !REPEAT_CAL) {
    // calibration data valid
    tft.setTouch(calData);
  } else {
    // data not valid so recalibrate
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.println("Touch corners as indicated");

    tft.setTextFont(1);
    tft.println();

    if (REPEAT_CAL) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println("Set REPEAT_CAL to false to stop this running again!");
    }

    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Calibration complete!");

    // store data
    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }
  }
}

void callback(char topic[], byte payload[], unsigned int l) {
  msgIn = "";
  for (int i = 0; i < l; i++) 
    msgIn = msgIn + ((char)payload[i]); 
 }


void publishMessage(char topic[], char msg[]){ 
  client.publish(topic, msg); 
}

void reconnectMqtt(char topic[]) {
  while (!client.connected()) { 
    if (client.connect(mqttCLIENT)) {
        client.subscribe(topic);
    }else
      delay(5000);
  }
}

void subscribeMessage(char topic[]){ 
  client.subscribe(topic); 
}

void emptyMatrix(){
  for(int i = 0; i<3; ++i)
    for(int j = 0; j<3; ++j)
      matrix[i][j] = '-';
}

boolean win(){
 
  char c = '-';
  for(int i = 0; i<3; ++i)
      if(String(matrix[i][0]).equals(String(matrix[i][1])) && String(matrix[i][0]).equals(String(matrix[i][2]))) {
        c = matrix[i][2];
        return (c=='O' || c=='X')?true:false;
      }

  for(int j = 0; j<3; ++j)
      if(String(matrix[0][j]).equals(String(matrix[1][j])) && String(matrix[0][j]).equals(String(matrix[2][j]))){ 
        c = matrix[2][j];
        return (c=='O' || c=='X')?true:false;
      }
      
  if(String(matrix[0][0]).equals(String(matrix[1][1])) && String(matrix[0][0]).equals(String(matrix[2][2]))){
    c = matrix[2][2];
    return (c=='O' || c=='X')?true:false;
  }
  

  if(String(matrix[0][2]).equals(String(matrix[1][1])) && String(matrix[0][2]).equals(String(matrix[2][0]))){
    c = matrix[0][2];
    return (c=='O' || c=='X')?true:false;
  }
 
  return false;
}

boolean parity(){
   for(int i = 0; i<3; ++i)
      for(int j = 0; j<3; ++j)
        if(String(matrix[i][j]).equals("-")) return false;
   return true;
}
