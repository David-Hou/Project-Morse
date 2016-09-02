//-------------------------------------------------------
// Global Variables
// Version 1.5 this is most likely to be the final version
// add the send part
#define MAJOR_MODE_RECEIVE 0
#define MAJOR_MODE_SEND 1
#define MINOR_MODE_WELCOME 0
#define MINOR_MODE_FUNCTION 1
#define MINOR_MODE_DISPLAY 2
#define CHATACTER_TABLE_SIZE 36
#define MAXCODELENGTH 100
#define MAXSOUNDSLENGTH 100
#define MAXDISPLAYLENGTH 50
struct _sound {
  int len;
  int type;
};

int LCD1602_RS = 12;
int LCD1602_RW = 11;
int LCD1602_EN = 10;
int DB[] = { 6, 7, 8, 9 };
int send_button_pin = 2;//button 的读取的引脚
const int major_control_pin = 3;
const int minor_control_pin = 4;
const int receive_sound_pin = 5;
const int light_pin = A0;
int major_mode;
int minor_mode;
int signal_state;
int sound_cnt;
int cur_time_interval;
int cur_time;
int prev_time;
char code[MAXCODELENGTH];
int code_len;
char display_content[MAXDISPLAYLENGTH];
int send_unit_time_interval = 800;
int send_prev_time;
int send_cur_time;
int send_prev_state;
int send_cur_state;
char send_code[MAXCODELENGTH];
int send_code_len;
char send_display_content[MAXDISPLAYLENGTH];
int send_welcome_finished = 0;
int receive_welcome_finished = 0;
int sound_finished = 0;
const char *CHARACTER_TABLE[CHATACTER_TABLE_SIZE] = {
  ".-",      //'A'
  "-...",    //'B'
  "-.-.",    //'C'
  "-..",     //'D'
  ".",       //'E'
  "..-.",    //'F'
  "--.",     //'G'
  "....",    //'H'
  "..",      //'I'
  ".---",    //'J'
  "-.-",     //'K'
  ".-..",    //'L'
  "--",      //'M'
  "-.",      //'N'
  "---",     //'O'
  ".--.",    //'P'
  "--.-",    //'Q'
  ".-.",     //'R'
  "...",     //'S'
  "-",       //'T'
  "..-",     //'U'
  "...-",    //'V'
  ".--",     //'W'
  "-..-",    //'X'
  "-.--",    //'Y'
  "--..",    //'Z'
  "-----",   //'0'
  ".----",   //'1'
  "..---",   //'2'
  "...--",   //'3'
  "....-",   //'4'
  ".....",   //'5'
  "-....",   //'6'
  "--...",   //'7'
  "---..",   //'8'
  "----.",   //'9'
};

struct _sound sounds[MAXSOUNDSLENGTH];
int sounds_len = 0;
//-------------------------------------------------
// The Function Used

void send_mode_welcome();
void receive_mode_welcome();
int sound2code(struct _sound * sounds, int sounds_len, char * code, int max_code_len);
int string2code(const char *string, int string_size, char *code, int max_code_size);
int code2binary(const char *code, int code_size, boolean *binary, int max_binary_size);
int code2string(const char *code, int code_size, char*string, int max_string_size);
int stringlen(const char* string);
int code2display(const char*code, int code_size, char*display, int max_display_size);
void LCD_init();
void LCD_display_content(const char* display_content, int size);
void LCD_Command_Write(int command);
void LCD_Data_Write(int dat);
void LCD_SET_XY(int x, int y);
void LCD_Write_Char(int x, int y, int dat);
void LCD_Write_String(int X, int Y, char *s);
void LCD_init();
void LCD_display(const char* send_display_content, int size);
void setup() {
  pinMode(major_control_pin, INPUT_PULLUP);
  pinMode(minor_control_pin, INPUT_PULLUP);
  pinMode(send_button_pin, INPUT_PULLUP);
  pinMode(receive_sound_pin, INPUT);
  pinMode(light_pin, INPUT);
  Serial.begin(9600);
  LCD_init();
}
void loop() {
  int major_control_signal = digitalRead(major_control_pin);
  //  Serial.print("major_control_signal:");
  //  Serial.println(major_control_signal);
  major_control_signal = 1 - major_control_signal;
  
  int minor_control_signal = digitalRead(minor_control_pin);
  minor_control_signal = 1 - minor_control_signal;
  if (!(minor_mode == 0 && major_mode==0)) {
    receive_welcome_finished = 0;
  }
  if (!(minor_mode == 0 && major_mode == 1)) {
    send_welcome_finished = 0;
  }
  if (!(major_mode == 1 && minor_mode == 1)) {
    Serial.print("mode:");
    Serial.print(major_mode);
    Serial.print(".");
    Serial.println(minor_mode);
  }
  if (minor_mode == 1) {
    digitalWrite(light_pin, HIGH);
  }
  else {
    digitalWrite(light_pin, LOW);
  }

  if (major_control_signal == 1) {
    major_mode = (major_mode + 1) % 2;
    delay(300);
  }
  if (minor_control_signal == 1) {
    minor_mode = (minor_mode + 1) % 3;
    delay(300);
  }
  if (major_mode == MAJOR_MODE_RECEIVE) {
    if (minor_mode == MINOR_MODE_WELCOME) {
      sounds_len = 0;
      sound_cnt = 0;
      sound_finished = 0;
      if (!receive_welcome_finished) {
        receive_mode_welcome();
        receive_welcome_finished = 1;
      }
    }
    else if (minor_mode == MINOR_MODE_FUNCTION) {
      get_sounds();
    }
    else if (minor_mode == MINOR_MODE_DISPLAY) {
      if (!sound_finished) {
        sound2code(sounds + 2, sounds_len - 2, code, MAXCODELENGTH);
        int display_content_size = code2display(code, code_len, display_content, MAXDISPLAYLENGTH);
        sound_finished = 1;
      }
      for (int i = 0; display_content[i] != 0; i++) {
        Serial.print(display_content[i]);
      }
      Serial.println();
      LCD_Command_Write(0x01);
      delay(50);
      LCD_Write_String(0, 0, display_content);
      delay(100);
    }
  }
  else if (major_mode == MAJOR_MODE_SEND) {
    if (minor_mode == MINOR_MODE_WELCOME) {
      if (!send_welcome_finished) {
        send_mode_welcome();
        send_welcome_finished = 1;
      }
    
      
    }
    else if (minor_mode == MINOR_MODE_FUNCTION) {
      //-------------------------send---------------------------------------------
      int send_button_volt;
      send_button_volt = digitalRead(send_button_pin);
      send_button_volt = 1 - send_button_volt;
      
      //  Serial.println(button_volt);
      if (send_button_volt == 1) {
        send_cur_state = 1;
      }
      else if (send_button_volt == 0) {
        send_cur_state = 0;
      }
      if (send_prev_state != send_cur_state) {
        // the state have been changed
        send_cur_time = millis();
        int send_cur_time_interval = send_cur_time - send_prev_time;

        Serial.println(send_cur_time_interval);
        send_prev_time = send_cur_time;
        // compare the current time with the standard
        if (send_prev_state == 0) {
          // previous state is 0
          // three circumstances: 1 unit interval 3 unit interval 7 unit interval
          if (send_cur_time_interval <= 700) {
          }
          else {
            // type in a ' '
            send_code[send_code_len++] = ' ';
          }
        }
        else if (send_prev_state == 1) {
          // two circumstances 1 unit 3 unit
          if (send_cur_time_interval <= 200) {
            send_code[send_code_len++] = '.';
          }
          else {
            send_code[send_code_len++] = '-';
          }
        }
        Serial.print("cur_state:");
        Serial.print(send_cur_state);
        Serial.println();
        Serial.print("prev_state:");
        Serial.print(send_prev_state);
        Serial.println();
        send_prev_state = send_cur_state;
        //------------------------------------------------
        // the display part
        int send_display_len = code2display(send_code, send_code_len, send_display_content, MAXDISPLAYLENGTH);
        send_display_content[send_display_len] = 0;
        Serial.print("display_len:");
        Serial.println(send_display_len);
        Serial.print("display_content:");
        for (int i = 0; i < send_display_len; i++) {
          Serial.print(send_display_content[i]);
        }
        Serial.println();
        //LCD_Command_Write(0x01);
        //delay(50);
        //LCD_Write_String(0, 0, send_display_content);
        //delay(50);
        LCD_display(send_display_content, send_display_len);
        for (int i = 0; i < send_code_len; i++) {
          Serial.print(send_code[i]);
        }
        Serial.println();

        //-------------------------send----------------------------------------
      }
    }
  }
}
void send_mode_welcome() {
  char str1[] = "Send Mode";
  char str2[] = "Welcome!";
  LCD_Command_Write(0x01);
  delay(50);
  LCD_Write_String(0, 0, str1);
  delay(50);
  LCD_Write_String(0, 1, str2);
  delay(50);
}
void receive_mode_welcome() {
  char str1[] = "Receive Mode";
  char str2[] = "Welcome!";
  LCD_Command_Write(0x01);
  delay(50);
  LCD_Write_String(0, 0, str1);
  delay(50);
  LCD_Write_String(0, 1, str2);
  delay(50);
}
int sound2code(struct _sound * sounds, int sounds_len, char * code, int max_code_len) {
  int seperator_0 = 255;
  int seperator_1 = 400;
  if (max_code_len < sounds_len) {
    return -1;
  }
  for (int i = 0; i < sounds_len; i++) {
    int len = sounds[i].len;
    int type = sounds[i].type;
    if (type == 0) {
      // three circumstances: 1 unit interval 3 unit interval 7 unit interval
      if (len <= seperator_0) {
      }
      else {
        // type in a ' '
        code[code_len++] = ' ';
      }
    }
    else if (type == 1) {
      // two circumstances 1 unit 3 unit
      if (len <= seperator_1) {
        code[code_len++] = '.';
      }
      else {
        code[code_len++] = '-';
      }
    }
  }
  return code_len;
}
//-----------------------------------------------------------------------
// The Function implemention
void LCD_init() {
  int i = 0;
  for (i = 6; i <= 12; i++)
  {
    pinMode(i, OUTPUT);
  }
  delay(100);
  LCD_Command_Write(0x28);//4线 2行 5x7
  delay(50);
  LCD_Command_Write(0x06);
  delay(50);
  LCD_Command_Write(0x0c);
  delay(50);
  LCD_Command_Write(0x80);
  delay(50);
  LCD_Command_Write(0x01);
  delay(50);
}
int code2string(const char *code, int code_size, char*string, int max_string_size) {
  int i;
  char cur[10];
  int cur_len = 0;
  int string_len = 0;
  bool unknown = false;
  // TODO:check cur pos whether exceed the max position
  for (i = 0; i < code_size; i++) {
    if (code[i] == '.' || code[i] == '-') {
      cur[cur_len++] = code[i];
      if (cur_len > 10) {
        //invalid number
        unknown = true;
      }
    }
    else if (code[i] == '?') {
      unknown = true;
    }
    else if (code[i] == ' ' || code[i] == '/') {
      // seperate the character
      // find the corresponding character now
      //debug
      // for (int i = 0; i < cur_len; i++) {
      //   printf("%c", cur[i]);
      // }
      // printf("\n");
      // debug
      if (unknown == true && code[i] == ' ') {
        // encounter an unknown character
        string[string_len++] = '?';
        unknown = false;
        cur_len = 0;
        continue;
        //move to the next
      }
      int j;
      for (j = 0; j < CHATACTER_TABLE_SIZE; j++) {
        int k;
        // cur_pos is the length of current morse code
        for (k = 0; k < cur_len; k++) {
          if (cur[k] != CHARACTER_TABLE[j][k]) {
            break;
          }
          // if exactly the same k == curPos
        }

        if (k == cur_len && cur_len == stringlen(CHARACTER_TABLE[j])) {
          // find it
          char cur_string_ch;
          if (j >= 0 && j < 26) {
            cur_string_ch = j + 'a';
          }
          else if (j >= 26 && j < 36) {
            cur_string_ch = '0' + j - 26;
          }
          string[string_len++] = cur_string_ch;
          break;
        }
      }
      if (j == CHATACTER_TABLE_SIZE) {
        // don't find such character error
        //TODO: error!
        //TODO 1319 test

        string[string_len++] = '?';
      }
      if (code[i] == ' ') {
        //nothing happend
      }
      else if (code[i] == '/') {
        string[string_len++] = ' ';
      }
      cur_len = 0;
    }
  }
  //TODO:check for the remaining character
  if (cur_len != 0) {
    // the last character
    if (unknown == true) {
      string[string_len++] = '?';
      return string_len;
    }
    else {
      int j;
      for (j = 0; j < CHATACTER_TABLE_SIZE; j++) {
        int k;
        // cur_pos is the length of current morse code
        for (k = 0; k < cur_len; k++) {
          if (cur[k] != CHARACTER_TABLE[j][k]) {
            break;
          }
          // if exactly the same k == curPos
        }

        if (k == cur_len && cur_len == stringlen(CHARACTER_TABLE[j])) {
          // find it
          char cur_string_ch;
          if (j >= 0 && j < 26) {
            cur_string_ch = j + 'a';
          }
          else if (j >= 26 && j < 36) {
            cur_string_ch = '0' + j - 26;
          }
          string[string_len++] = cur_string_ch;
          break;
        }
      }
      if (j == CHATACTER_TABLE_SIZE) {
        // don't find such character error
        //TODO: error!
        string[string_len++] = '?';
      }
    }
  }
  return string_len;
}
int string2code(const char* string, int string_size, char *code, int max_code_size) {
  // 可以接受的string只能带有数字以及小写字母 如果遇到其他字母或者溢出 return -1 else return 0
  int i;
  int code_pos = 0;
  int code_len = 0;
  for (i = 0; i < string_size; i++) {
    char ch = string[i];
    if (ch >= 'a' && ch <= 'z') {
      //TODO: 这边ch 不是字母时候考虑一下
      int ch_int = string[i] - 'a'; // the integer corresponding to the character
      const char * ch_pointer = CHARACTER_TABLE[ch_int];
      while (*ch_pointer) {
        if (code_len > max_code_size) {
          return -1;
        }
        code[code_len] = *ch_pointer;
        code_len++;
        ch_pointer++;
      }
      if (i != string_size - 1 && string[i + 1] != ' ') {
        if (code_len > max_code_size) {
          return -1;
        }
        code[code_len++] = ' '; // seperater between character
      }
    }
    // 以上 弄完了一个字符
    // 停顿用 _ 表示
    else if (ch == ' ') {
      code[code_len++] = '/'; // add three more unit interval and counts to 6 unit interval
    }
    else if (ch >= '0' && ch <= '9') {
      int ch_int = string[i] - '0' + 26; // the integer corresponding to the character
      const char * ch_pointer = CHARACTER_TABLE[ch_int];
      while (*ch_pointer) {
        if (code_len > max_code_size) {
          return -1;
        }
        code[code_len] = *ch_pointer;
        code_len++;
        ch_pointer++;
      }
      if (i != string_size - 1 && string[i + 1] != ' ') {
        if (code_len > max_code_size) {
          return -1;
        }
        code[code_len++] = ' '; // seperater between character
      }
    }
    else {
      //TODO: other circumstances
      return -1;
    }
  }
  return code_len;
}
int code2binary(const char *code, int code_size, boolean* binary, int max_binary_size) {
  int i;
  // code contains '.' '-' ' ' '/'
  int binary_len = 0;
  for (i = 0; i < code_size; i++) {
    if (code[i] == '.') {
      //TODO:CHECK
      //add a 1 and a 0  (0 is the operator between binary)
      if (binary_len + 2 > max_binary_size) {
        return -1;
      }
      binary[binary_len++] = true;
      //TODO: check for the last character not to add more 0s
      if (code[i + 1] != ' ' && code[i + 1] != '/') {
        binary[binary_len++] = false;
      }
    }
    else if (code[i] == '-') {
      if (binary_len + 4 > max_binary_size) {
        return -1;
      }
      binary[binary_len++] = true;
      binary[binary_len++] = true;
      binary[binary_len++] = true;
      //TODO: check for the last character not to add more 0s
      if (code[i + 1] != ' ' && code[i + 1] != '/') {
        binary[binary_len++] = false;
      }
    }
    else if (code[i] == ' ') {
      if (binary_len + 3 > max_binary_size) {
        return -1;
      }
      binary[binary_len++] = false;
      binary[binary_len++] = false;
      binary[binary_len++] = false;
    }
    else if (code[i] == '/') {
      if (binary_len + 7 > max_binary_size) {
        return -1;
      }
      binary[binary_len++] = false;
      binary[binary_len++] = false;
      binary[binary_len++] = false;
      binary[binary_len++] = false;
      binary[binary_len++] = false;
      binary[binary_len++] = false;
      binary[binary_len++] = false;
    }
  }
  return binary_len;
}
int stringlen(const char *string) {
  int len = 0;
  while (*string) {
    string++;
    len++;
  }
  return len;
}
int code2display(const char*code, int code_size, char*display, int max_display_size) {
  // if success return 0 else return -1
  int i;
  for (i = code_size - 1; i >= 0; i--) {
    if (code[i] == ' ' || code[i] == '/') {
      break;
    }
  }
  int display_prev_len = i;
  // code[i] == ' ' || code[i] == '/'
  int display_len = code2string(code, display_prev_len + 1, display, max_display_size);
  if (display_len == -1) {
    return -1;
  }
  for (i = display_prev_len + 1; i < code_size; i++) {
    display[display_len++] = code[i];
    if (display_len > max_display_size) {
      return -1;
    }
  }
  return display_len;
}

void LCD_display_content(const char *display_content, int size) {
  // 16 * 2
  // find the last 2 * 16
  int line = (size - 1) / 16;
  if (line == 0) {
    char first_line_content[17] = { 0 };
    Serial.println("in LCDdisplay");
    for (int i = 0; i < size; i++) {
      first_line_content[i] = display_content[i];
      LCD_Command_Write(0x01);
      delay(30);
      LCD_Write_String(0, 0, first_line_content);
      delay(50);
    }
  }
  else {
    // 48 1 2
    // 47 0 1 2 16~31
    int first_line_no = line - 1;
    int second_line_no = line;
    char first_line_content[17] = { 0 };
    char second_line_content[17] = { 0 };
    //copy
    int i, j;
    for (i = first_line_no * 16, j = 0; i < first_line_no * 16 + 16; i++, j++) {
      first_line_content[j] = display_content[i];
    }
    first_line_content[16] = 0;
    for (i = second_line_no * 16, j = 0; i < second_line_no * 16 + 16; i++, j++) {
      second_line_content[j] = display_content[i];
    }
    second_line_content[16] = 0;
    Serial.println("in LCDdisplay");
    LCD_Command_Write(0x01);
    delay(30);
    LCD_Write_String(0, 0, first_line_content);
    delay(50);
    LCD_Write_String(0, 1, second_line_content);
    delay(50);
  }
}
void LCD_Command_Write(int command)
{
  int i, temp;
  digitalWrite(LCD1602_RS, LOW);
  digitalWrite(LCD1602_RW, LOW);
  digitalWrite(LCD1602_EN, LOW);

  temp = command & 0xf0;
  for (i = DB[0]; i <= 9; i++)
  {
    digitalWrite(i, temp & 0x80);
    temp <<= 1;
  }

  digitalWrite(LCD1602_EN, HIGH);
  delayMicroseconds(1);
  digitalWrite(LCD1602_EN, LOW);

  temp = (command & 0x0f) << 4;
  for (i = DB[0]; i <= 9; i++)
  {
    digitalWrite(i, temp & 0x80);
    temp <<= 1;
  }

  digitalWrite(LCD1602_EN, HIGH);
  delayMicroseconds(1);
  digitalWrite(LCD1602_EN, LOW);
}
void LCD_Data_Write(int dat)
{
  int i = 0, temp;
  digitalWrite(LCD1602_RS, HIGH);
  digitalWrite(LCD1602_RW, LOW);
  digitalWrite(LCD1602_EN, LOW);

  temp = dat & 0xf0;
  for (i = DB[0]; i <= 9; i++)
  {
    digitalWrite(i, temp & 0x80);
    temp <<= 1;
  }

  digitalWrite(LCD1602_EN, HIGH);
  delayMicroseconds(1);
  digitalWrite(LCD1602_EN, LOW);

  temp = (dat & 0x0f) << 4;
  for (i = DB[0]; i <= 9; i++)
  {
    digitalWrite(i, temp & 0x80);
    temp <<= 1;
  }

  digitalWrite(LCD1602_EN, HIGH);
  delayMicroseconds(1);
  digitalWrite(LCD1602_EN, LOW);
}
void LCD_SET_XY(int x, int y)
{
  int address;
  if (y == 0)    address = 0x80 + x;
  else          address = 0xC0 + x;
  LCD_Command_Write(address);
}
void LCD_Write_Char(int x, int y, int dat)
{
  LCD_SET_XY(x, y);
  LCD_Data_Write(dat);
}
void LCD_Write_String(int X, int Y, char *s)
{
  LCD_SET_XY(X, Y);    //设置地址
  while (*s)             //写字符串
  {
    LCD_Data_Write(*s);
    s++;
  }
}

void get_sounds() {
  int cur_time;
  int sound_signal = digitalRead(receive_sound_pin);
  
  if (sound_signal == 1) {
    if (sound_cnt < 50) {
      sound_cnt += 14;
    }
  }
  else if (sound_signal == 0) {
    if (sound_cnt > -30) {
      sound_cnt -= 3;
    }
  }
  if (sound_cnt > 0) {
    if (signal_state == 0) {
      sound_cnt = 50;
      signal_state = 1;
      cur_time = millis();
      cur_time_interval = cur_time - prev_time;
      sounds_len++;
      prev_time = cur_time;
    }
  }
  else if (sound_cnt <= 0) {
    if (signal_state == 1) {
      sound_cnt = -30;
      signal_state = 0;
      cur_time = millis();
      cur_time_interval = cur_time - prev_time;
      sounds_len++;
      prev_time = cur_time;
    }
  }
  sounds[sounds_len].len = cur_time_interval;
  sounds[sounds_len].type = 1 - signal_state;
}

void LCD_display(const char *send_display_content, int size) {
  // 16 * 2
  // find the last 2 * 16
  int line = (size - 1) / 16;
  if (line == 0) {
    char first_line_content[17] = { 0 };
    Serial.println("in LCDdisplay");
    for (int i = 0; i < size; i++) {
      first_line_content[i] = send_display_content[i];
    }
    LCD_Command_Write(0x01);
    delay(50);
    LCD_Write_String(0, 0, first_line_content);
    delay(50);
  }
  else {
    // 48 1 2
    // 47 0 1 2 16~31
    int first_line_no = line - 1;
    int second_line_no = line;
    char first_line_content[17] = { 0 };
    char second_line_content[17] = { 0 };
    //copy
    int i, j;
    for (i = first_line_no * 16, j = 0; i < first_line_no * 16 + 16; i++, j++) {
      first_line_content[j] = send_display_content[i];
    }
    first_line_content[16] = 0;
    for (i = second_line_no * 16, j = 0; i < second_line_no * 16 + 16; i++, j++) {
      second_line_content[j] = send_display_content[i];
    }
    second_line_content[16] = 0;
    Serial.println("in LCDdisplay");
    LCD_Command_Write(0x01);
    delay(50);
    LCD_Write_String(0, 0, first_line_content);
    delay(50);
    LCD_Write_String(0, 1, second_line_content);
    delay(50);
  }
}
