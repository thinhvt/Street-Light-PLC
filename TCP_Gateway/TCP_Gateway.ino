#include <SoftwareSerial.h>

SoftwareSerial mySerial(10, 11); // RX, TX

int8_t answer;
char aux_str[50];
char ip_data[30] = "21.77.0.0.0.1";
bool isConnected = false;
uint8_t notify[] = {10, 'C', 'o', 'n', 'n', 'e', 'c', 't', 'e', 'd'};
void setup() {
  pinMode(1, INPUT);
  pinMode(2, OUTPUT);
  pinMode(10, INPUT);
  pinMode(11, OUTPUT);
  pinMode(13, OUTPUT);
  Serial.begin(57600);
  mySerial.begin(9600);
  Serial.println("Starting...");
  sendATcommand2("AT+CIPSHUT", "SHUT OK", "ERROR", 10000);
  power_on();
  delay(3000);
  Serial.println("Connecting to the network...");
  connect_server();
}
void loop() {
  String response;
  char backStr[30];

  if (!isConnected)
  {
    connect_server();
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
      if (response.equals("CLOSE"))
      {
        connect_server();
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
        while (mySerial.available() > 0) mySerial.read();   // Clean the input buffer
        response = "";
      }
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
        if(backStr[0] == '\0')
          sprintf(backStr, "%s%s", backStr, b);
        else
          sprintf(backStr, "%s%s%s", backStr, "|", b);
        b[0] = '\0';
      }

      backStr[strlen(backStr)] = '\0';
      // Sends some data to the TCP socket
      int back_len = strlen(backStr);
      if (back_len > 0)
      {
        sprintf(aux_str, "AT+CIPSEND=%d", back_len);
        if (sendATcommand2(aux_str, ">", "ERROR", 10000) == 1)
        {
          sendATcommand2(backStr, "SEND OK", "ERROR", 10000);
        }
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
          if (sendATcommand2("AT+CIPSTART=\"TCP\",\"103.53.231.126\",\"10000\"",
                             "CONNECT OK", "CONNECT FAIL", 30000) == 1)
          {
            Serial.println("Connected");
            isConnected = true;
            delay(1000);

            mySerial.write((byte*)&notify, sizeof(notify));
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
