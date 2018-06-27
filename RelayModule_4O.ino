const int RELAY_NUM_OF_OUTPUT_MAX = 4;
const int RELAY_PORT_NUMBER_START = 22;
const int RELAY_PORT_NUMBER[RELAY_NUM_OF_OUTPUT_MAX] = {RELAY_PORT_NUMBER_START, RELAY_PORT_NUMBER_START+1, RELAY_PORT_NUMBER_START+2, RELAY_PORT_NUMBER_START+3};

const int LED_STATUS_PORT_NUMBER = 13;

const long SERIAL_BAUD_RATE = 115200;

boolean Relay_on[RELAY_NUM_OF_OUTPUT_MAX];

//const bool HEARTBEAT_ENABLED = true;
const bool HEARTBEAT_ENABLED = false;
const long HEARTBEAT_TIME_OUT = 1000; // unit is ms. 1000 = 1sec.
long Heartbeat_time_start; // unit is ms.
char Heartbeat_string[RELAY_NUM_OF_OUTPUT_MAX+2];

void setup() {
  Serial1.begin(SERIAL_BAUD_RATE);

  pinMode(LED_STATUS_PORT_NUMBER, OUTPUT);

  int i;

  for (i = 0; i < RELAY_NUM_OF_OUTPUT_MAX; i ++)
  {
    pinMode(RELAY_PORT_NUMBER[i] , OUTPUT);
    Relay_on[i] = false;
  }

  if (HEARTBEAT_ENABLED) {
    Heartbeat_string[0] = 'R';
    for (i = 0; i < RELAY_NUM_OF_OUTPUT_MAX; i ++)
    {
      Heartbeat_string[i + 1] = '0';
    }
    Heartbeat_string[i + 1] = '0';

    Heartbeat_time_start = millis();
  }
}

void loop() {
  //Serial1.print(count);
  for (int i = 0; i < RELAY_NUM_OF_OUTPUT_MAX; i ++) {
    //Serial1.print("RELAY_PORT_NUMBER["); Serial1.print(i); Serial1.print("]="); Serial1.print(RELAY_PORT_NUMBER[i]); Serial1.print("\n\r");
    if (Relay_on[i])  digitalWrite(RELAY_PORT_NUMBER[i], HIGH);
    else              digitalWrite(RELAY_PORT_NUMBER[i], LOW);
  }

  serial1Event();

  if (HEARTBEAT_ENABLED) {
    long timer = millis();
    long timer_diff;

    if (timer < Heartbeat_time_start)
      timer_diff = 0xffffffff - Heartbeat_time_start + timer;
    else
      timer_diff = timer - Heartbeat_time_start;
    if (timer_diff >= HEARTBEAT_TIME_OUT) {
      Heartbeat_time_start = millis();
      digitalWrite(LED_STATUS_PORT_NUMBER, HIGH);
      Serial1.print(Heartbeat_string);
      Serial1.flush();
      digitalWrite(LED_STATUS_PORT_NUMBER, LOW);
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

  while (Serial1.available()) {
    digitalWrite(LED_STATUS_PORT_NUMBER, HIGH);
    char read_char = (char)Serial1.read();
    //Serial1.print("state="); Serial1.print(state); Serial1.print("\n\r");
    do { // do while loop for continue statement on switch.
      switch (state) {
        case STATE_IDLE:
          if (read_char != 'R') break;
          state = STATE_DATA_n;
          check_sum = '0';
          break;
        case STATE_CHECK_SUM:
          int i;
          if (read_char != check_sum) {
            state = STATE_IDLE;
            continue;
          }
          if (HEARTBEAT_ENABLED) Heartbeat_string[0] = 'R';
          for (i = 0; i < RELAY_NUM_OF_OUTPUT_MAX; i ++) {
            //Serial1.print("data["); Serial1.print(i); Serial1.print("]="); Serial1.print(data[i]); Serial1.print("\n\r");
            if (data[i] == '1') Relay_on[i] = true;
            else                Relay_on[i] = false;
            if (HEARTBEAT_ENABLED) Heartbeat_string[i + 1] = data[i];
          }
          if (HEARTBEAT_ENABLED) {
            Heartbeat_string[i + 1] = check_sum;
            Heartbeat_time_start = millis();
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
  }
  digitalWrite(LED_STATUS_PORT_NUMBER, LOW);
}
