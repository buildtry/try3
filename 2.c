#include <pic.h>
#define _XTAL_FREQ 20000000  // 20MHz oscillator frequency

// LCD Connections (PORTD)
#define RS RD0
#define EN RD1
#define D4 RD2
#define D5 RD3
#define D6 RD4
#define D7 RD5

// Motor 1 Control Pins (using PORTB & PORTC)
#define M1_IN1 RB0
#define M1_IN2 RB1
#define M1_PWM RC2   // PWM via CCP1

// Motor 2 Control Pins (using PORTC)
#define M2_IN1 RC3
#define M2_IN2 RC4
#define M2_PWM RC1   // PWM via CCP2

// Push Button Inputs (PORTB)
#define SPEED_UP RB2
#define SPEED_DOWN RB3
#define DIR_CHANGE RB4

// Variables
int speed = 0;         // Speed percentage (0-100)
int direction = 1;     // 1 = CW, 0 = CCW

// Debounce state variables for each button
char lastStateSpeedUp = 1;
char lastStateSpeedDown = 1;
char lastStateDirChange = 1;

// Function Prototypes
void LCD_Command(unsigned char);
void LCD_Char(unsigned char);
void LCD_Init();
void LCD_String(const char*);
void LCD_Clear();
void Update_Display();
void Set_PWM(int duty);    // For Motor 1 using CCP1
void Set_PWM2(int duty);   // For Motor 2 using CCP2
void Check_Buttons();
char debounce_button(char button, char *lastState);

//-----------------------------------------------------
// LCD Functions
//-----------------------------------------------------
void LCD_Command(unsigned char cmd) {
    RS = 0;
    // Send upper nibble
    D4 = (cmd >> 4) & 1;
    D5 = (cmd >> 5) & 1;
    D6 = (cmd >> 6) & 1;
    D7 = (cmd >> 7) & 1;
    EN = 1; __delay_us(10); EN = 0;
    // Send lower nibble
    D4 = cmd & 1;
    D5 = (cmd >> 1) & 1;
    D6 = (cmd >> 2) & 1;
    D7 = (cmd >> 3) & 1;
    EN = 1; __delay_us(10); EN = 0;
    __delay_ms(2);
}

void LCD_Char(unsigned char data) {
    RS = 1;
    // Send upper nibble
    D4 = (data >> 4) & 1;
    D5 = (data >> 5) & 1;
    D6 = (data >> 6) & 1;
    D7 = (data >> 7) & 1;
    EN = 1; __delay_us(10); EN = 0;
    // Send lower nibble
    D4 = data & 1;
    D5 = (data >> 1) & 1;
    D6 = (data >> 2) & 1;
    D7 = (data >> 3) & 1;
    EN = 1; __delay_us(10); EN = 0;
    __delay_ms(2);
}

void LCD_Init() {
    TRISD = 0x00;  // Set PORTD as output for LCD
    __delay_ms(20);
    LCD_Command(0x02); // Return Home
    LCD_Command(0x28); // 4-bit mode, 2 lines, 5x8 font
    LCD_Command(0x0C); // Display on, cursor off
    LCD_Command(0x06); // Entry mode set
    LCD_Command(0x01); // Clear display
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

//-----------------------------------------------------
// PWM Functions
//-----------------------------------------------------
// Motor 1 PWM using CCP1 (full 10-bit resolution)
void Set_PWM(int duty) {
    unsigned int pwm_val;
    CCP1CON = 0x0C;  // Set PWM mode (DC1B bits to be updated)
    PR2 = 124;       // PWM period value
    pwm_val = ((unsigned long)duty * (4 * (PR2 + 1))) / 100;
    CCPR1L = pwm_val >> 2;                          // Upper 8 bits
    CCP1CON = (CCP1CON & 0xCF) | ((pwm_val & 0x03) << 4); // Lower 2 bits in bits 5-4
    T2CON = 0x04;    // Start Timer2
}

// Motor 2 PWM using CCP2 (full 10-bit resolution)
void Set_PWM2(int duty) {
    unsigned int pwm_val;
    CCP2CON = 0x0C;  // Set PWM mode for CCP2 (DC2B bits to be updated)
    // PR2 is shared with CCP1, so no need to reinitialize it here.
    pwm_val = ((unsigned long)duty * (4 * (PR2 + 1))) / 100;
    CCPR2L = pwm_val >> 2;                          // Upper 8 bits
    CCP2CON = (CCP2CON & 0xCF) | ((pwm_val & 0x03) << 4); // Lower 2 bits in bits 5-4
    // Timer2 already running
}

//-----------------------------------------------------
// Display Update Function
//-----------------------------------------------------
void Update_Display() {
    LCD_Clear();
    LCD_String("DUTY: ");
    if (speed == 100) {
        LCD_Char('1');
        LCD_Char('0');
        LCD_Char('0');
    } else {
        LCD_Char((speed / 10) + '0');
        LCD_Char((speed % 10) + '0');
    }
    LCD_Char('%');
    LCD_Command(0xC0);
    LCD_String("DIR: ");
    LCD_String(direction ? "CW " : "CCW");
}

//-----------------------------------------------------
// Debounce Function
//-----------------------------------------------------
char debounce_button(char button, char *lastState) {
    if (!button && *lastState) {  // Detect falling edge (button pressed)
        __delay_ms(50);           // Debounce delay
        *lastState = 0;
        return 1;
    } else if (button) {
        *lastState = 1;
    }
    return 0;
}

//-----------------------------------------------------
// Button Check and Update Function
//-----------------------------------------------------
void Check_Buttons() {
    char updateFlag = 0;
    
    if (debounce_button(SPEED_UP, &lastStateSpeedUp) && speed < 100) {
        speed += 10;
        updateFlag = 1;
    }
    if (debounce_button(SPEED_DOWN, &lastStateSpeedDown) && speed > 0) {
        speed -= 10;
        updateFlag = 1;
    }
    if (debounce_button(DIR_CHANGE, &lastStateDirChange)) {
        direction = !direction;
        updateFlag = 1;
    }
    
    if (updateFlag) {
        // Update PWM for both motors
        Set_PWM(speed);
        Set_PWM2(speed);
        
        // Update motor directions for both motors
        if (direction) {
            // Clockwise: set IN1 high and IN2 low for both motors
            M1_IN1 = 1; M1_IN2 = 0;
            M2_IN1 = 1; M2_IN2 = 0;
        } else {
            // Counter-clockwise: set IN1 low and IN2 high for both motors
            M1_IN1 = 0; M1_IN2 = 1;
            M2_IN1 = 0; M2_IN2 = 1;
        }
        Update_Display();
    }
}

//-----------------------------------------------------
// Main Function
//-----------------------------------------------------
void main() {
    // Configure button inputs (PORTB)
    TRISB2 = 1;
    TRISB3 = 1;
    TRISB4 = 1;
    
    // Configure Motor 1 control pins
    TRISB0 = 0;    // M1_IN1
    TRISB1 = 0;    // M1_IN2
    TRISC2 = 0;    // M1_PWM (CCP1)
    
    // Configure Motor 2 control pins (PORTC)
    TRISC1 = 0;    // M2_PWM (CCP2)
    TRISC3 = 0;    // M2_IN1
    TRISC4 = 0;    // M2_IN2
    
    OPTION_REGbits.nRBPU = 0;  // Enable internal pull-ups for PORTB
    
    // Immediately clear PWM outputs to ensure 0% duty cycle at startup
    M1_PWM = 0;
    CCPR1L = 0;
    CCP1CON &= 0xCF;  // Clear lower 2 bits for Motor 1 PWM
    M2_PWM = 0;
    CCPR2L = 0;
    CCP2CON &= 0xCF;  // Clear lower 2 bits for Motor 2 PWM
    
    // Disable motor outputs until initialization is complete
    M1_IN1 = 0; M1_IN2 = 0;
    M2_IN1 = 0; M2_IN2 = 0;
    
    // Initialize debounce states (assuming buttons are pulled high)
    lastStateSpeedUp = 1;
    lastStateSpeedDown = 1;
    lastStateDirChange = 1;
    
    __delay_ms(50);  // Allow hardware to settle
    
    LCD_Init();
    Set_PWM(speed);    // Initialize Motor 1 PWM (0% duty)
    Set_PWM2(speed);   // Initialize Motor 2 PWM (0% duty)
    Update_Display();
    
    // Now enable motor control pins for default direction (CW)
    direction = 1;
    M1_IN1 = 1; M1_IN2 = 0;
    M2_IN1 = 1; M2_IN2 = 0;
    
    while (1) {
        Check_Buttons();
    }
}
