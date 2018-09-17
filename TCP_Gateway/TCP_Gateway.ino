#include <SoftwareSerial.h>

SoftwareSerial mySerial(10, 11); // RX, TX

int8_t answer;
char aux_str[50];
uint8_t server_connected;
uint8_t con_str[6] = {6, 'C', 'n', 'n', 't', 'd'};
char chk_conn[11] = "Connecting";
String response;
char backStr[100];
uint8_t notice_plm;

void setup() {
  pinMode(1, INPUT);
  pinMode(2, OUTPUT);
  pinMode(10, INPUT);
  pinMode(11, OUTPUT);
  pinMode(13, OUTPUT);

  server_connected = 1;
  notice_plm = 1;
  Serial.begin(57600);
  mySerial.begin(9600);
  Serial.println("Starting...");
  sendATcommand2("AT+CIPSHUT", "OK", "ERROR", 10000);
  power_on();
  delay(3000);
  Serial.println("Connecting to the network...");
  connect_server();
}
void loop() {
  if (server_connected > 0)
  {
    sendATcommand2("AT+CIPSHUT", "OK", "ERROR", 10000);
    connect_server();
    delay(1000);
  }
  else
  {
    if (Serial.available() > 0)
    {
      digitalWrite(13, HIGH);
      delay(300);
      digitalWrite(13, LOW);

      response = Serial.readString();
      response.trim();
      if (response.equals("CLOSED"))
      {
        server_connected = 1;
        Serial.println("Server closed connection! Trying to re-connect...");
        sendATcommand2("AT+CIPSHUT", "OK", "ERROR", 10000);
        connect_server();
      }
      else if(response.equals("ChkConnection"))
      {
        int conn_len = strlen(chk_conn);
        sprintf(aux_str, "AT+CIPSEND=%d", conn_len);
        if (sendATcommand2(aux_str, ">", "ERROR", 10000) == 1)
        {
          sendATcommand2(chk_conn, "SEND OK", "ERROR", 10000);
        }
        while (Serial.available() > 0) Serial.read();   // Clean the input buffer
      }
      else
      {
        int len = response.length();
        int index = 0;
        uint8_t buff[len / 2];

        for (int i = 0; i < len; i += 2)
        {
          char temp[3];
          response.substring(i, i + 2).toCharArray(temp, 3);
          temp[2] = '\0';
          buff[index] = strtoul(temp, NULL, 16);
          index++;
        }

        mySerial.write((byte*)&buff, sizeof(buff));
        mySerial.flush();
      }
      response = "";
    }

    else if (mySerial.available() > 0)
    {
      uint8_t back_index = 0;
      char b[4];
      backStr[0] = '\0';

      while (mySerial.available() > 0)
      {
        uint8_t c = mySerial.read();
        String(c).toCharArray(b, 4);
        if (backStr[0] == '\0')
          sprintf(backStr, "%s%s", backStr, b);
        else
          sprintf(backStr, "%s%s%s", backStr, "|", b);
        b[0] = '\0';
      }

      if(notice_plm == 0)
      {
        sprintf(backStr, "%s%s", backStr, "?Connected");
        notice_plm = 1;
      }
      backStr[strlen(backStr)] = '\0'; //Terminate string

      int back_len = strlen(backStr);
      // Sends data to the TCP socket
      if (back_len > 0 && strcmp(backStr, "255") != 0)
      {
        sprintf(aux_str, "AT+CIPSEND=%d", back_len);
        if (sendATcommand2(aux_str, ">", "ERROR", 10000) == 1)
        {
          sendATcommand2(backStr, "SEND OK", "ERROR", 10000);
        }
        while (Serial.available() > 0) Serial.read();   // Clean the input buffer
      }
    }
  }
}

void power_on() {
  uint8_t answer = 0;

  // checks if the module is started
  answer = sendATcommand2("AT", "OK", "OK", 2000);
  if (answer == 0)
  {
    // power on pulse
    digitalWrite(2, HIGH);
    delay(3000);
    digitalWrite(2, LOW);

    // waits for an answer from the module
    while (answer == 0) {   // Send AT every two seconds and wait for the answer
      answer = sendATcommand2("AT", "OK", "OK", 2000);
    }
  }
  // Selects Single-connection mode
  if (sendATcommand2("AT+CIPMUX=0", "OK", "ERROR", 1000) == 1)
  {
    // Waits for status IP INITIAL
    while (sendATcommand2("AT+CIPSTATUS", "INITIAL", "", 500)  == 0 );
  }
  else
  {
    Serial.println("Error setting the single connection");
  }
}

void connect_server()
{
  if (sendATcommand2("AT+CGATT=1", "OK", "ERROR", 1000) == 1)
  {
    // Sets the APN, user name and password
    if (sendATcommand2("AT+CSTT=\"internet\",\"\",\"\"", "OK",  "ERROR", 30000) == 1)
    {
      // Waits for status IP START
      while (sendATcommand2("AT+CIPSTATUS", "START", "", 500)  == 0 );
      // Brings Up Wireless Connection
      if (sendATcommand2("AT+CIICR", "OK", "ERROR", 30000) == 1)
      {
        // Waits for status IP GPRSACT
        while (sendATcommand2("AT+CIPSTATUS", "GPRSACT", "", 500)  == 0 );
        // Gets Local IP Address
        if (sendATcommand2("AT+CIFSR", ".", "ERROR", 10000) == 1)
        {
          // Waits for status IP STATUS
          while (sendATcommand2("AT+CIPSTATUS", "IP STATUS", "", 500)  == 0 );
          Serial.println("Openning TCP");
          // Opens a TCP socket
          if (sendATcommand2("AT+CIPSTART=\"TCP\",\"103.53.231.126\",\"777\"",
                             "CONNECT OK", "CONNECT FAIL", 30000) == 1)
          {
            server_connected = 0;
            notice_plm = 0;
            delay(1000);
//            sprintf(aux_str, "AT+CIPSEND=%d", 18);
//            if (sendATcommand2(aux_str, ">", "ERROR", 10000) == 1)
//            {
//              sendATcommand2("13|21|77?Connected", "SEND OK", "ERROR", 10000);
//            }

            mySerial.write((byte*)&con_str, sizeof(con_str));
            mySerial.flush();
            while (Serial.available() > 0) Serial.read();   // Clean the input buffer
            while (mySerial.available() > 0) mySerial.read();   // Clean the input buffer
          }
          else
          {
            Serial.println("Error openning the connection");
          }
        }
        else
        {
          Serial.println("Error getting the IP address");
        }
      }
      else
      {
        Serial.println("Error bring up wireless connection");
      }
    }
    else
    {
      Serial.println("Error setting the APN");
    }
  }
  else
  {
    Serial.println("Error enable GPRS");
  }
}

int8_t sendATcommand2(char* ATcommand, char* expected_answer1,
                      char* expected_answer2, unsigned int timeout) {
  uint8_t x = 0,  answer = 0;
  char response[100];
  unsigned long previous;

  memset(response, "", 100);    // Initialize the string

  delay(100);

  while ( Serial.available() > 0) Serial.read();   // Clean the input buffer

  Serial.println(ATcommand);    // Send the AT command

  x = 0;
  previous = millis();

  // this loop waits for the answer
  do {
    // if there are data in the UART input buffer, reads it and checks for the asnwer
    if (Serial.available() != 0) {
      response[x] = Serial.read();
      x++;
      // check if the desired answer 1  is in the response of the module
      if (strstr(response, expected_answer1) != NULL)
      {
        answer = 1;
      }
      // check if the desired answer 2 is in the response of the module
      else if (strstr(response, expected_answer2) != NULL)
      {
        answer = 2;
      }
    }
  }
  // Waits for the asnwer with time out
  while ((answer == 0) && ((millis() - previous) < timeout));

  return answer;
}
