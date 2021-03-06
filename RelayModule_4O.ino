const char VERSION[] = "1.10";
const char DATE[] = "2018-07-26";

const int RELAY_NUM_OF_OUTPUT_MAX = 4;
const int RELAY_PORT_NUMBER_START = 22;
const int RELAY_PORT_NUMBER[RELAY_NUM_OF_OUTPUT_MAX] = {RELAY_PORT_NUMBER_START, RELAY_PORT_NUMBER_START+1, RELAY_PORT_NUMBER_START+2, RELAY_PORT_NUMBER_START+3};

const int LED_STATUS_PORT_NUMBER = 13;

const long SERIAL_BAUD_RATE = 115200;

boolean Relay_on[RELAY_NUM_OF_OUTPUT_MAX];

const bool HEARTBEAT_ENABLED = true;
//const bool HEARTBEAT_ENABLED = false;
const unsigned long HEARTBEAT_TIME_OUT_DEFAULT = 1000; // unit is ms. 1000 = 1sec.
const unsigned long HEARTBEAT_TIME_OUT_RECEIVED = 25; // unit is ms. 1000 = 1sec.
const int HEARTBEAT_TIME_OUT_RECEIVED_CNT = 10; // Status LED blinking count when data received.
unsigned long Heartbeat_time_start; // unit is ms.
unsigned long Heartbeat_time_out_value; // unit is ms.
int Heartbeat_time_out_reset_cnt;

const bool WATCHDOG_ENABLED = true;
//const bool WATCHDOG_ENABLED = false;
const unsigned long WATCHDOG_TIME_OUT = 5000; // unit is ms. 5000 = 5sec.
unsigned long Watchdog_time_start; // unit is ms.

//const bool AUTO_REPLY_ENABLED = true;
const bool AUTO_REPLY_ENABLED = false;
const unsigned long AUTO_REPLY_TIME_OUT = 1000; // unit is ms. 1000 = 1sec.
unsigned long Auto_Reply_time_start; // unit is ms.
char Auto_Reply_string[RELAY_NUM_OF_OUTPUT_MAX+2+1]; // +2 = Header and checksum, +1 = null

char Reply_Requested_string[RELAY_NUM_OF_OUTPUT_MAX+2+1]; // +2 = Header and checksum, +1 = null

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  Serial1.begin(SERIAL_BAUD_RATE);

  pinMode(LED_STATUS_PORT_NUMBER, OUTPUT);

  int i;

  for (i = 0; i < RELAY_NUM_OF_OUTPUT_MAX; i ++)
  {
    pinMode(RELAY_PORT_NUMBER[i] , OUTPUT);
    Relay_on[i] = false;
  }

  if (WATCHDOG_ENABLED) {
    Watchdog_time_start = millis();
  }

  if (AUTO_REPLY_ENABLED) {
    Auto_Reply_string[0] = 'R';
    for (i = 0; i < RELAY_NUM_OF_OUTPUT_MAX; i ++)
    {
      Auto_Reply_string[i + 1] = '0';
    }
    Auto_Reply_string[i + 1] = '0';
    Auto_Reply_string[i + 2] = 0;

    Auto_Reply_time_start = millis();
  }

  Reply_Requested_string[RELAY_NUM_OF_OUTPUT_MAX + 2] = 0;

  if (HEARTBEAT_ENABLED) {
    Heartbeat_time_start = millis();
    Heartbeat_time_out_value = HEARTBEAT_TIME_OUT_DEFAULT;
    Heartbeat_time_out_reset_cnt = 0;
  }

  Serial.print("Version:");
  Serial.print(VERSION);
  Serial.print(" ");
  Serial.print("Date:");
  Serial.print(DATE);
  Serial.print("\n\r");
  Serial.flush();
}

void loop() {
  //Serial1.print(count);
  for (int i = 0; i < RELAY_NUM_OF_OUTPUT_MAX; i ++) {
    //Serial1.print("RELAY_PORT_NUMBER["); Serial1.print(i); Serial1.print("]="); Serial1.print(RELAY_PORT_NUMBER[i]); Serial1.print("\n\r");
    if (Relay_on[i])  digitalWrite(RELAY_PORT_NUMBER[i], HIGH);
    else              digitalWrite(RELAY_PORT_NUMBER[i], LOW);
  }

  serial1Event();

  if (WATCHDOG_ENABLED) {
    unsigned long time = millis();
    unsigned long time_diff;

    if (time < Watchdog_time_start)
      time_diff = 0xffffffff - Watchdog_time_start + time;
    else
      time_diff = time - Watchdog_time_start;
    if (time_diff >= WATCHDOG_TIME_OUT) {
      Watchdog_time_start = millis();
      Serial.print("W");
      for (int i = 0; i < RELAY_NUM_OF_OUTPUT_MAX; i ++)
      {
        Relay_on[i] = false;
        Serial.print("0");
      }
      Serial.print("\n\r");
      Serial.flush();
    }
  }

  if (AUTO_REPLY_ENABLED) {
    unsigned long time = millis();
    unsigned long time_diff;

    if (time < Auto_Reply_time_start)
      time_diff = 0xffffffff - Auto_Reply_time_start + time;
    else
      time_diff = time - Auto_Reply_time_start;
    if (time_diff >= AUTO_REPLY_TIME_OUT) {
      Auto_Reply_time_start = millis();
      Serial1.print(Auto_Reply_string);
      Serial1.flush();
    }
  }

  if (HEARTBEAT_ENABLED) {
    unsigned long time = millis();
    unsigned long time_diff;

    if (time < Heartbeat_time_start)
      time_diff = 0xffffffff - Heartbeat_time_start + time;
    else
      time_diff = time - Heartbeat_time_start;
    if (time_diff >= Heartbeat_time_out_value) {
      digitalWrite(
        LED_STATUS_PORT_NUMBER,
        (digitalRead(LED_STATUS_PORT_NUMBER) == HIGH)?LOW:HIGH);
      Heartbeat_time_start = millis();
      if (Heartbeat_time_out_reset_cnt) {
        Heartbeat_time_out_reset_cnt --;
      }
      else {
        Heartbeat_time_out_value = HEARTBEAT_TIME_OUT_DEFAULT;
      }
    }
  }
}

const int STATE_IDLE = 0;
const int STATE_DATA_n = 1;
const int STATE_CHECK_SUM = RELAY_NUM_OF_OUTPUT_MAX + 1;

void serial1Event() {
  static int state = STATE_IDLE; // Idle
  static char data[RELAY_NUM_OF_OUTPUT_MAX];
  static char check_sum;
  static bool reply_requested = false;

  while (Serial1.available()) {
    char read_char = (char)Serial1.read();
    //Serial1.print("state="); Serial1.print(state); Serial1.print("\n\r");
    do { // do while loop for continue statement on switch.
      switch (state) {
        case STATE_IDLE:
          if (read_char != 'R'
              &&
              read_char != 'Q') break;
          if (read_char == 'Q') reply_requested = true;
          else                  reply_requested = false;
          state = STATE_DATA_n;
          check_sum = '0';
          break;
        case STATE_CHECK_SUM:
          int i;
          if (read_char != check_sum) {
            state = STATE_IDLE;
            continue;
          }
          if (WATCHDOG_ENABLED) {
            Watchdog_time_start = millis();
          }
          if (AUTO_REPLY_ENABLED) Auto_Reply_string[0] = 'R';
          if (reply_requested) {
            Reply_Requested_string[0] = 'Q';
            Serial.print("Q");
          }
          else {
            Serial.print("R");
          }
          for (i = 0; i < RELAY_NUM_OF_OUTPUT_MAX; i ++) {
            //Serial1.print("data["); Serial1.print(i); Serial1.print("]="); Serial1.print(data[i]); Serial1.print("\n\r");
            if (data[i] == '1') Relay_on[i] = true;
            else                Relay_on[i] = false;
            if (AUTO_REPLY_ENABLED) Auto_Reply_string[i + 1] = data[i];
            if (reply_requested) Reply_Requested_string[i + 1] = data[i];
            Serial.print(data[i]);
          }
          Serial.print(check_sum);
          Serial.print("\n\r");
          Serial.flush();
          if (AUTO_REPLY_ENABLED) {
            Auto_Reply_string[i + 1] = check_sum;
            Auto_Reply_time_start = millis();
          }
          if (reply_requested) {
            Reply_Requested_string[i + 1] = check_sum;
            Serial1.print(Reply_Requested_string);
            Serial1.flush();
          }
          if (HEARTBEAT_ENABLED) {
            Heartbeat_time_out_value = HEARTBEAT_TIME_OUT_RECEIVED;
            Heartbeat_time_out_reset_cnt = HEARTBEAT_TIME_OUT_RECEIVED_CNT;
          }
          state = STATE_IDLE;
          break;
        default: // for STATE_DATA_n:
          if (read_char != '0' && read_char != '1') {
            state = STATE_IDLE;
            continue;
          }
          data[state - STATE_DATA_n] = read_char;
          check_sum += read_char - '0';
          //Serial1.print("check_sum="); Serial1.print(check_sum, DEC); Serial1.print("\n\r");
          state ++;
          break;
      }
      break;
    } while (true);
  } // End of while (Serial1.available())
}
