const int StatusLED = 13;
const int Relay_Output_MAX = 4;
const int Relay_Number_Start = 22;
const int Relay_Number[Relay_Output_MAX] = {Relay_Number_Start, Relay_Number_Start+1, Relay_Number_Start+2, Relay_Number_Start+3};
const long HeartBit_Timer_Max = 1000; // unit is ms. 1000 = 1sec.

boolean Relay_on[Relay_Output_MAX] = {false, false, false, false};
long HeartBit_Timer_Start; // unit is ms.
char HeartBit_String[Relay_Output_MAX+2] = {'R', '0', '0', '0', '0', '0'};

void setup() {
  Serial1.begin(115200);
  pinMode(StatusLED, OUTPUT);
  for (int i = 0; i < Relay_Output_MAX; i ++)
    pinMode(Relay_Number[i] , OUTPUT);
  HeartBit_Timer_Start = millis();
}

void loop() {
  //Serial1.print(count);
  for (int i = 0; i < Relay_Output_MAX; i ++) {
    //Serial1.print("Relay_Number["); Serial1.print(i); Serial1.print("]="); Serial1.print(Relay_Number[i]); Serial1.print("\n\r");
    if (Relay_on[i])  digitalWrite(Relay_Number[i], HIGH);
    else              digitalWrite(Relay_Number[i], LOW);
  }

  serial1Event();

  long timer = millis();
  long timer_diff;

  if (timer < HeartBit_Timer_Start)
    timer_diff = 0xffffffff - HeartBit_Timer_Start + timer;
  else
    timer_diff = timer - HeartBit_Timer_Start;
  if (timer_diff >= HeartBit_Timer_Max) {
    HeartBit_Timer_Start = millis();
    digitalWrite(StatusLED, HIGH);
    Serial1.print(HeartBit_String);
    Serial1.flush();
    digitalWrite(StatusLED, LOW);
  }
}

const int state_IDLE = 0;
const int state_DATA_n = 1;
const int state_CHECK_SUM = Relay_Output_MAX + 1;
int state = state_IDLE; // Idle
char data[4] = {0, 0, 0, 0};
char sum = '0';

void serial1Event() {
  while (Serial1.available()) {
    digitalWrite(StatusLED, HIGH);
    char inChar = (char)Serial1.read();
    //Serial1.print("state="); Serial1.print(state); Serial1.print("\n\r");
    do {
      switch (state) {
        case state_IDLE:
          if (inChar != 'R') break;
          state = state_DATA_n;
          sum = '0';
          break;
        case state_CHECK_SUM:
          int i;
          if (inChar != sum) {
            state = state_IDLE;
            continue;
          }
          for (i = 0; i < Relay_Output_MAX; i ++) {
            //Serial1.print("data["); Serial1.print(i); Serial1.print("]="); Serial1.print(data[i]); Serial1.print("\n\r");
            if (data[i] == '1') Relay_on[i] = true;
            else                Relay_on[i] = false;
            HeartBit_String[i + 1] = data[i];
          }
          HeartBit_String[i + 1] = sum;
          HeartBit_Timer_Start = millis();
          Serial1.print(HeartBit_String);
          state = state_IDLE;
          break;
        default: // for state_DATA_n:
          if (inChar != '0' && inChar != '1') {
            state = state_IDLE;
            continue;
          }
          data[state - state_DATA_n] = inChar;
          sum += inChar - '0';
          //Serial1.print("sum="); Serial1.print(sum, DEC); Serial1.print("\n\r");
          state ++;
          break;
      }
      break;
    } while (true);
  }
  digitalWrite(StatusLED, LOW);
}
