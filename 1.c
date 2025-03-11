#include <htc.h>
#define _XTAL_FREQ 20000000  // Define oscillator frequency (20MHz)

// LCD Connections
#define RS RD0
#define EN RD1
#define D4 RD2
#define D5 RD3
#define D6 RD4
#define D7 RD5

// Motor Control Pins
#define IN1 RB0
#define IN2 RB1
#define PWM RC2

// Push Button Inputs
#define SPEED_UP RB2
#define SPEED_DOWN RB3
#define DIR_CHANGE RB4

// Variables
int speed = 0;  // Initial speed (0)
int direction = 1; // 1 = CW, 0 = CCW
char lastButtonState = 0;

// Function Prototypes
void LCD_Command(unsigned char);
void LCD_Char(unsigned char);
void LCD_Init();
void LCD_String(const char*);
void LCD_Clear();
void Set_PWM(int);
void Update_Display();
void Check_Buttons();
char debounce_button(char button);

// LCD Functions
void LCD_Command(unsigned char cmd) {
    RS = 0;
    D4 = (cmd >> 4) & 1;
    D5 = (cmd >> 5) & 1;
    D6 = (cmd >> 6) & 1;
    D7 = (cmd >> 7) & 1;
    EN = 1; __delay_us(10); EN = 0;
    D4 = cmd & 1;
    D5 = (cmd >> 1) & 1;
    D6 = (cmd >> 2) & 1;
    D7 = (cmd >> 3) & 1;
    EN = 1; __delay_us(10); EN = 0;
    __delay_ms(2);
}

void LCD_Char(unsigned char data) {
    RS = 1;
    D4 = (data >> 4) & 1;
    D5 = (data >> 5) & 1;
    D6 = (data >> 6) & 1;
    D7 = (data >> 7) & 1;
    EN = 1; __delay_us(10); EN = 0;
    D4 = data & 1;
    D5 = (data >> 1) & 1;
    D6 = (data >> 2) & 1;
    D7 = (data >> 3) & 1;
    EN = 1; __delay_us(10); EN = 0;
    __delay_ms(2);
}

void LCD_Init() {
    TRISD = 0x00;
    __delay_ms(20);
    LCD_Command(0x02);
    LCD_Command(0x28);
    LCD_Command(0x0C);
    LCD_Command(0x06);
    LCD_Command(0x01);
}

void LCD_String(const char *str) {
    while (*str) {
        LCD_Char(*str++);
    }
}

void LCD_Clear() {
    LCD_Command(0x01);
    __delay_ms(2);
}

// PWM Setup
void Set_PWM(int duty) {
    CCP1CON = 0x0C;
    PR2 = 124;
    CCPR1L = (duty * 124) / 100;
    T2CON = 0x04;
}

// Display Update
void Update_Display() {
    LCD_Clear();
    LCD_String("DUTY: ");
    LCD_Char((speed / 10) + '0');
    LCD_Char((speed % 10) + '0');
    LCD_Char('%');
    LCD_Command(0xC0);
    LCD_String("DIR: ");
    LCD_String(direction ? "CCW " : "CW");
}

// Button Debounce Function
char debounce_button(char button) {
    static char lastState = 1;
    if (!button && lastState) {  // Detect falling edge (press)
        __delay_ms(50);
        lastState = 0;
        return 1;
    } else if (button) {
        lastState = 1;
    }
    return 0;
}

// Button Check
void Check_Buttons() {
    char updateFlag = 0;

    if (debounce_button(SPEED_UP) && speed < 100) {
        speed += 10;
        updateFlag = 1;
    }
    if (debounce_button(SPEED_DOWN) && speed > 0) {
        speed -= 10;
        updateFlag = 1;
    }
    if (debounce_button(DIR_CHANGE)) {
        direction = !direction;
        IN1 = direction;
        IN2 = !direction;
        updateFlag = 1;
    }

    if (updateFlag) {
        Set_PWM(speed);
        Update_Display();
    }
}

// Main Function
void main() {
    TRISB = 0x1C;  // RB2, RB3, RB4 as inputs
    TRISC2 = 0;    // PWM output
    TRISB0 = 0;
    TRISB1 = 0;
    
    OPTION_REGbits.nRBPU = 0;  // Enable internal pull-ups for buttons

    LCD_Init();
    Set_PWM(speed);
    Update_Display();

    while (1) {
        Check_Buttons();
    }
}
