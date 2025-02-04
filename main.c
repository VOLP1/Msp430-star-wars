#include <msp430.h>
#include <stdio.h>

// DefiniÃ§Ãµes I2C LCD
#define LCD_I2C_ADDR 0x3F  // EndereÃ§o I2C padrÃ£o do mÃ³dulo LCD
#define LCD_BACKLIGHT 0x08
#define LCD_ENABLE 0x04
#define LCD_COMMAND 0x00
#define LCD_DATA 0x01

//definicao LEDS
#define LED1 (0x0001)
#define LED2 (0x0080)

//DEFINICOES CHAVES
#define SW1 BIT1        //CHAVE SW1
#define SW1_IN  P2IN
#define SW2 BIT1        //CHAVE SW2
#define SW2_IN P1IN

//DEFINICOES LOOP PRINCIPAL
#define VERDADEIRO 1
#define FALSO 0
#define PINO  BIT5
#define PINO_OUT  P2OUT
#define MAXIMOVALOR 63429U  //CCR0 BASE PARA 0,1ms
#define REBOTE 2000    //delay PARA EVITAR BOUNCE
#define ABERTA 1
#define FECHADA 0

//DEFINICAO DA FREQUENCIA EM HZ DAS NOTAS MUSICAIS
#define c 261
#define d 294
#define e 329
#define f 349
#define g 391
#define gS 415
#define a 440
#define aS 455
#define b 466
#define cH 523
#define cSH 554
#define dH 587
#define dSH 622
#define eH 659
#define fH 698
#define fSH 740
#define gH 784
#define gSH 830
#define aH 880

//VARIAVEIS DO TEMPO
int decimossegundos;
int segundos;
int minutos;
int horas;

//CARACTERES DAS LINHAS
char primeiraLinha[20];
char segundaLinha[20];

//MODIFICADORES DE ESTADO DO PROGRAMA
int running;
int flag_linha_1;
int flag_linha_2;

//ESTADO DAS CHAVES
int estado_chave_1=ABERTA;
int estado_chave_2=ABERTA;

//PERGUNTAS
char *line1_questions[5]= {"Quem o Kylo   ","Kylo Ren e o que", "qual o planeta  ","Quem enfrenta Va","Qual o ultimo"};
char *line2_questions[5]= {"Ren mata?     ","de Han Solo    ?", "natal de Luke?  ","der em Mustafar?","Sith criado?"};
char *line1_options[5]= {"A:LEIA B:HANSOLO","A:PAI B:FILHO","A:Naboo B:Corusc","A:KitB:MaceWindu","A:Maul B:Sidious"};
char *line2_options[5]= {"C:LUKE D:REY","C:TIO D:SOBRINHO", "C:TatoineD:Jakku","C:ObiWan D:Yoda","C:Ren D:Dookan"};
int question_answer[5]={1,1,2,2,2};
unsigned int actualQuestion = 0;
int questionMode = 0;
int numberOfPoints = 0;
unsigned int letterState = 0;
unsigned int selectedLetter = 0;

// FunÃ§Ãµes I2C para LCD
void i2c_init(void) {
    P3SEL |= BIT0 + BIT1;                  // Atribui pinos I2C (P3.0 = SDA, P3.1 = SCL)

    UCB0CTL1 |= UCSWRST;                   // Enable SW reset
    UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;  // I2C Master, synchronous mode
    UCB0CTL1 = UCSSEL_2 + UCSWRST;         // Use SMCLK, keep SW reset
    UCB0BR0 = 12;                          // fSCL = SMCLK/12 = ~100kHz
    UCB0BR1 = 0;
    UCB0I2CSA = LCD_I2C_ADDR;              // LCD address
    UCB0CTL1 &= ~UCSWRST;                  // Clear SW reset, resume operation

    // Habilitar resistores de pull-up internos (opcional, caso não use externos)
    P3REN |= BIT0 + BIT1;                  // Habilita resistores
    P3OUT |= BIT0 + BIT1;                  // Pull-up
}

void i2c_write(unsigned char data) {
    while (UCB0CTL1 & UCTXSTP);
    UCB0CTL1 |= UCTR + UCTXSTT;

    while (!(UCB0IFG & UCTXIFG));           // Aguarda buffer TX vazio
    UCB0TXBUF = data;

    while (!(UCB0IFG & UCTXIFG));           // Aguarda conclusÃ£o do TX

    UCB0CTL1 |= UCTXSTP;
    while (UCB0CTL1 & UCTXSTP);
}

void lcd_send_cmd(unsigned char cmd) {
    unsigned char data_u, data_l;
    data_u = cmd & 0xf0;
    data_l = (cmd << 4) & 0xf0;

    i2c_write(data_u | LCD_BACKLIGHT);
    i2c_write(data_u | LCD_ENABLE | LCD_BACKLIGHT);
    __delay_cycles(5);
    i2c_write(data_u | LCD_BACKLIGHT);
    __delay_cycles(200);

    i2c_write(data_l | LCD_BACKLIGHT);
    i2c_write(data_l | LCD_ENABLE | LCD_BACKLIGHT);
    __delay_cycles(5);
    i2c_write(data_l | LCD_BACKLIGHT);
    __delay_cycles(2000);
}

void lcd_init(void) {
    __delay_cycles(40000);

    lcd_send_cmd(0x03);
    __delay_cycles(5000);
    lcd_send_cmd(0x03);
    __delay_cycles(150);
    lcd_send_cmd(0x03);
    lcd_send_cmd(0x02);

    lcd_send_cmd(0x28);        // 4-bit mode, 2 lines
    lcd_send_cmd(0x0C);        // Display ON, cursor OFF
    lcd_send_cmd(0x06);        // Auto increment cursor
    lcd_send_cmd(0x01);        // Clear display

    __delay_cycles(2000);
}

void lcd_send_data(unsigned char data) {
    unsigned char data_u, data_l;
    data_u = data & 0xf0;
    data_l = (data << 4) & 0xf0;

    i2c_write(data_u | LCD_DATA | LCD_BACKLIGHT);
    i2c_write(data_u | LCD_DATA | LCD_ENABLE | LCD_BACKLIGHT);
    __delay_cycles(5);
    i2c_write(data_u | LCD_DATA | LCD_BACKLIGHT);
    __delay_cycles(200);

    i2c_write(data_l | LCD_DATA | LCD_BACKLIGHT);
    i2c_write(data_l | LCD_DATA | LCD_ENABLE | LCD_BACKLIGHT);
    __delay_cycles(5);
    i2c_write(data_l | LCD_DATA | LCD_BACKLIGHT);
    __delay_cycles(2000);
}

void lcd_send_string(char *str) {
    while(*str) lcd_send_data(*str++);
}

void lcd_set_cursor(unsigned char row, unsigned char col) {
    unsigned char cmd;
    cmd = (row == 0) ? 0x80 + col : 0xC0 + col;
    lcd_send_cmd(cmd);
}

void lcd_clear(void) {
    lcd_send_cmd(0x01);
    __delay_cycles(2000);
}

// FunÃ§Ãµes de delay
void delay_ms(unsigned int ms) {
    unsigned int i;
    for (i = 0; i<= ms; i++)
       __delay_cycles(500);
}

void delay_us(unsigned int us) {
    unsigned int i;
    for (i = 0; i<= us/2; i++)
       __delay_cycles(1);
}

void delay(unsigned long timeValue) {
    volatile unsigned long cont=timeValue;
    while(cont != 0) {
        cont--;
    }
}

// FunÃ§Ãµes do buzzer
void beep(unsigned int note, unsigned int duration) {
    int i;
    long delay = (long)(10000/note);
    long time = (long)((duration*100)/(delay*2));
    for (i=0;i<time;i++) {
        P1OUT |= BIT2;
        delay_us(delay);
        P1OUT &= ~BIT2;
        delay_us(delay);
    }
    delay_ms(20);
}

void play()
{
    beep(a, 500);
    delay_ms(100);
    beep(a, 500);
    delay_ms(100);
    beep(a, 500);
    delay_ms(100);
    beep(f, 350);
    delay_ms(100);
    beep(cH, 150);
    delay_ms(100);
    beep(a, 500);
    delay_ms(100);
    beep(f, 350);
    delay_ms(100);
    beep(cH, 150);
    delay_ms(100);
    beep(a, 650);
    delay_ms(100);

    delay_ms(350);
    //INVERTALO ENTRE PRIMEIRA E SEGUNDA PARTE

    beep(eH, 500);
    delay_ms(100);
    beep(eH, 500);
    delay_ms(100);
    beep(eH, 500);
    delay_ms(100);
    beep(fH, 350);
    delay_ms(100);
    beep(cH, 150);
    delay_ms(100);
    beep(gS, 500);
    delay_ms(100);
    beep(f, 350);
    delay_ms(100);
    beep(cH, 150);
    delay_ms(100);
    beep(a, 650);

    delay_ms(350);

    beep(aH, 500);
    delay_ms(100);
    beep(a, 300);
    delay_ms(100);
    beep(a, 150);
    delay_ms(100);
    beep(aH, 400);
    delay_ms(100);
    beep(gSH, 200);
    delay_ms(100);
    beep(gH, 200);
    delay_ms(100);
    beep(fSH, 125);
    delay_ms(100);
    beep(fH, 125);
    delay_ms(100);
    beep(fSH, 250);

    delay_ms(250);

    beep(aS, 250);
    delay_ms(100);
    beep(dSH, 400);
    delay_ms(100);
    beep(dH, 200);
    delay_ms(100);
    beep(cSH, 200);
    delay_ms(100);
    beep(cH, 125);
    delay_ms(100);
    beep(b, 125);
    delay_ms(100);
    beep(cH, 250);

    delay_ms(250);

    beep(f, 125);
    delay_ms(100);
    beep(gS, 500);
    delay_ms(100);
    beep(f, 375);
    delay_ms(100);
    beep(a, 125);
    delay_ms(100);
    beep(cH, 500);
    delay_ms(100);
    beep(a, 375);
    delay_ms(100);
    beep(cH, 125);
    delay_ms(100);
    beep(eH, 650);

    beep(aH, 500);
    delay_ms(100);
    beep(a, 300);
    delay_ms(100);
    beep(a, 150);
    delay_ms(100);
    beep(aH, 400);
    delay_ms(100);
    beep(gSH, 200);
    delay_ms(100);
    beep(gH, 200);
    delay_ms(100);
    beep(fSH, 125);
    delay_ms(100);
    beep(fH, 125);
    delay_ms(100);
    beep(fSH, 250);

    delay_ms(250);

    beep(aS, 250);
    delay_ms(100);
    beep(dSH, 400);
    delay_ms(100);
    beep(dH, 200);
    delay_ms(100);
    beep(cSH, 200);
    delay_ms(100);
    beep(cH, 125);
    delay_ms(100);
    beep(b, 125);
    delay_ms(100);
    beep(cH, 250);

    delay_ms(250);

    beep(f, 250);
    delay_ms(100);
    beep(gS, 500);
    delay_ms(100);
    beep(f, 375);
    delay_ms(50);
    beep(cH, 125);
    delay_ms(100);
    beep(a, 500);
    delay_ms(100);
    beep(f, 375);
    delay_ms(100);
    beep(cH, 125);
    delay_ms(100);
    beep(a, 650);

}

void timer_a_configuracao(void) {
    TA0CTL = TBSSEL_1 |  MC_1  |  ID_2;
    TA0CCR0 = MAXIMOVALOR;
    TA0CCTL0 = CCIE;
}

void portas_configuracao(void) {
    P2DIR |= PINO;
    PINO_OUT &= ~PINO;
    P2DIR &= ~SW1;
    P2REN |= SW1;
    P2OUT |= SW1;
    P1DIR &= ~SW2;
    P1REN |= SW2;
    P1OUT |= SW2;
}

void config_leds(void) {
    P1DIR |= LED1;
    P1OUT &= (~LED1);
    P4DIR |= LED2;
    P4OUT &= (~LED2);
}

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;

    P1DIR |= BIT2;
    P2DIR |= BIT5;
    P2DIR |= BIT4;

    running = FALSO;
    flag_linha_1 = VERDADEIRO;
    flag_linha_2 = VERDADEIRO;

    // InicializaÃ§Ã£o do LCD I2C
    i2c_init();
    lcd_init();

    // ConfiguraÃ§Ãµes do sistema
    portas_configuracao();
    timer_a_configuracao();
    config_leds();
    _enable_interrupt();

  while(1) {
      //ESCREVE NA LINHA 1 DO LCD
      if (flag_linha_1) {
          lcd_set_cursor(0, 0);  // Primeira linha, primeira coluna
          switch(questionMode) {
              case(0):
                  sprintf(primeiraLinha, "%s", line1_questions[actualQuestion]);
                  break;
              case(1):
                  sprintf(primeiraLinha, "%s", line1_options[actualQuestion]);
                  break;
              case(2):
                  sprintf(primeiraLinha, "Sua pontuacao:");
                  break;
              case(3):
                  if(selectedLetter == question_answer[actualQuestion]) {
                      P2OUT |= BIT4;
                      if(selectedLetter == 0) {
                          sprintf(primeiraLinha, "Acertou letra A");
                      } else if(selectedLetter == 1) {
                          sprintf(primeiraLinha, "Acertou letra B");
                      } else if(selectedLetter == 2) {
                          sprintf(primeiraLinha, "Acertou letra C");
                      } else if(selectedLetter == 3) {
                          sprintf(primeiraLinha, "Acertou letra D");
                      }
                  } else {
                      if(question_answer[actualQuestion] == 0) {
                          P2OUT |= BIT5;
                          beep(aH, 2000);
                          sprintf(primeiraLinha, "Errou letra A");
                      } else if(question_answer[actualQuestion] == 1) {
                          P2OUT |= BIT5;
                          beep(aH, 2000);
                          sprintf(primeiraLinha, "Errou letra B");
                      } else if(question_answer[actualQuestion] == 2) {
                          P2OUT |= BIT5;
                          beep(aH, 2000);
                          sprintf(primeiraLinha, "Errou letra C");
                      } else if(question_answer[actualQuestion] == 3) {
                          P2OUT |= BIT5;
                          beep(aH, 2000);
                          sprintf(primeiraLinha, "Errou letra D");
                      }
                  }
                  break;
          }
          lcd_send_string(primeiraLinha);
          lcd_send_string("   ");
      }

      //ESCREVE NA LINHA 2 DO LCD
      if(flag_linha_2) {
          lcd_set_cursor(1, 0);  // Segunda linha, primeira coluna
          switch(questionMode) {
              case(0):
                  sprintf(segundaLinha, "%s", line2_questions[actualQuestion]);
                  break;
              case(1):
                  sprintf(segundaLinha, "%s", line2_options[actualQuestion]);
                  break;
              case(2):
                  sprintf(segundaLinha, "%d pontos      ", numberOfPoints);
                  break;
              case(3):
                  if(question_answer[actualQuestion] == 0) {
                      sprintf(segundaLinha, "%s", line1_options[actualQuestion]);
                  } else if(question_answer[actualQuestion] == 1) {
                      sprintf(segundaLinha, "%s", line1_options[actualQuestion]);
                  } else if(question_answer[actualQuestion] == 2) {
                      sprintf(segundaLinha, "%s", line2_options[actualQuestion]);
                  } else if(question_answer[actualQuestion] == 3) {
                      sprintf(segundaLinha, "%s", line2_options[actualQuestion]);
                  }
                  break;
          }
          lcd_send_string(segundaLinha);
          lcd_send_string("   ");
      }

      //VERIFICA CLIQUE NA CHAVE 2
      if ((SW2_IN & SW2) == 0) {
          if (estado_chave_2 == ABERTA) {
              estado_chave_2 = FECHADA;
              switch(letterState) {
                  case(0):
                      letterState = 1;
                      P1OUT &= (~LED1);
                      P4OUT |= (LED2);
                      break;
                  case(1):
                      letterState = 2;
                      P1OUT |= (LED1);
                      P4OUT &= (~LED2);
                      break;
                  case(2):
                      letterState = 3;
                      P1OUT |= (LED1);
                      P4OUT |= (LED2);
                      break;
                  case(3):
                      letterState = 0;
                      P1OUT &= (~LED1);
                      P4OUT &= (~LED2);
                      break;
              }
          }
      } else {
          estado_chave_2 = ABERTA;
      }

      //VERIFICA CLIQUE NA CHAVE 1
      if((SW1_IN & SW1) == 0) {
          if (estado_chave_1 == ABERTA) {
              estado_chave_1 = FECHADA;
              selectedLetter = letterState;
              P1OUT &= (~LED1);
              P4OUT &= (~LED2);

              delay(5000);
              P1OUT |= (LED1);
              P4OUT |= (LED2);
              delay(5000);
              P1OUT &= (~LED1);
              P4OUT &= (~LED2);
              delay(5000);
              P1OUT |= (LED1);
              P4OUT |= (LED2);
              delay(5000);
              P1OUT &= (~LED1);
              P4OUT &= (~LED2);

              questionMode = 3;
          }
      } else {
          estado_chave_1 = ABERTA;
      }
  }
}
#pragma vector=TIMER0_A0_VECTOR;
__interrupt void timera0_inte(void) {
    P2OUT &= ~BIT4;
    P2OUT &= ~BIT5;
    if (questionMode == 0) {
        questionMode = 1;
    } else {
        if (questionMode == 1 || questionMode == 3) {
            if (selectedLetter == question_answer[actualQuestion]) {
                numberOfPoints += 100;
            }
            actualQuestion++;
            if (actualQuestion < 5) {
                questionMode = 0;
            } else {
                questionMode = 2;
            }
        } else {
            P2OUT |= BIT5;
            P2OUT |= BIT4;
            delay_ms(4000);
            play();
        }
    }
}
