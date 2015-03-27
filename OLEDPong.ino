/* N1202Test --- test the 128x64 pixel OLED on Arduino      2015-03-22 */
/* Copyright (c) 2015 John Honniball */

#include <math.h>
#include <SPI.h>
#include "font.h"

// Direct port I/O defines for Arduino with ATmega328
// Change these if running on Mega Arduino
#define OLEDOUT PORTB
#define CS      0x04 // AVR Pin B2
#define DC      0x10 // AVR Pin B4
#define SDA     0x08
#define SCLK    0x20

// Connections to 128x64 OLED, via CD4050 level-shifter
#define slaveSelectPin 10
#define RSTPin 9
#define DCPin 12
#define SDAPin 11
#define SCLKPin 13

#define SWPIN 2  // Switch (on joystick)

// Size of 128x64 OLED screen
#define MAXX 128
#define MAXY 64
#define MAXROWS 8

// Co-ord of centre of screen
#define CENX (MAXX / 2)
#define CENY (MAXY / 2)

#define LEFTBAT 0
#define RIGHTBAT (MAXX - 2)

#define PLAYER 1  // Player seves ball
#define AI 2      // AI serves ball

// AI actions when ball on player's side of net
#define STAY 0    // AI does not move
#define JIGGLE 1  // AI moves up and down
#define SHADOW 2  // AI matches player movement
#define CENTRE 3  // AI moves towards centre of screen

// SSD1306 command bytes
#define SSD1306_SETCONTRAST 0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF

#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA

#define SSD1306_SETVCOMDETECT 0xDB

#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9

#define SSD1306_SETMULTIPLEX 0xA8

#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10

#define SSD1306_SETSTARTLINE 0x40

#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR   0x22

#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8

#define SSD1306_SEGREMAP 0xA0

#define SSD1306_CHARGEPUMP 0x8D

// The frame buffer, 1024 bytes
unsigned char Frame[MAXROWS][MAXX];

void setup (void)
{
  int r, c;
  long int before, after;
  
  Serial.begin (9600);
  
  Serial.println ("OLED 128x64 Pong");

  oled_begin ();
  
  greyFrame ();
  
  textRoundRect ("PONG");
  
  updscreen ();
  
  delay (1500);
}


void loop (void)
{
  long int start;
  int msg = 0;
  
  do {
    greyFrame ();
    
    switch (msg) {
    case 0:
      textRoundRect ("DEPOSIT QUARTER");
      break;
    case 1:
      textRoundRect2 ("BALL WILL SERVE", "AUTOMATICALLY");
      break;
    case 2:
      textRoundRect2 ("AVOID MISSING BALL", "FOR HIGH SCORE");
      break;
    }
    
    if (msg == 2)
      msg = 0;
    else
      msg++;
    
    updscreen ();
    
    start = millis ();
    
    while (digitalRead (SWPIN) == HIGH)
      if ((millis () - start) > 2000)
        break;
  } while (digitalRead (SWPIN) == HIGH);
  
  playPong ();
}


void playPong (void)
{
  static char over[] = "GAME OVER";
  int serve = PLAYER;
  int pscore = 0;
  int ascore = 0;
  
  do {
    if (playOneBall (serve, pscore, ascore) == PLAYER) {
      serve = PLAYER;
      pscore++;
    }
    else {
      serve = AI;
      ascore++;
    }
  } while ((pscore < 15) && (ascore < 15));
  
  delay (1000);
  
  greyFrame ();
  
  if (pscore == 15)
    textRoundRect2 (over, "PLAYER WINS");
  else
    textRoundRect2 (over, "COMPUTER WINS");
  
  updscreen ();
  
  delay (1500);
}

int playOneBall (int serve, int pscore, int ascore)
{
  int baty, aibaty;
  int bx, by;
  int bvx, bvy;
  unsigned int frame;
  long int start, now;
  int elapsed;
  int winner = 0;
  
  // Ball position, served from centre of net
  bx = CENX * 16;
  by = CENY * 16;
  
  bvx = 0;
  bvy = 0;
  
  aibaty = CENY * 16;
  
  for (frame = 0; winner == 0; frame++) {
    // Record timer in milliseconds at start of frame cycle
    start = millis ();
    
    // Draw empty playing area (i.e. the net)
    drawBackground ();

    // Get player's position
    baty = analogRead (0);
    
    if (baty < (8 * 16))
      baty = 8 * 16;
      
    if (baty > ((MAXY - 8) * 16))
      baty = (MAXY - 8) * 16;
    
    drawBat (LEFTBAT, baty);
  
    // AI for the right-hand bat
    aibaty = airightbat (aibaty, bx, by, bvx, baty);
    
    drawBat (RIGHTBAT, aibaty);
    
    drawBall (bx, by);
  
    drawScores (pscore, ascore);
    
    updscreen ();
    
    if (frame == 25) {
      if (serve == AI) {
        bvx = -3 * 16;
        bvy = 8;
      }
      else {
        bvx = 3 * 16;
        bvy = (baty - (CENY * 16)) / 16;
      }
    }
    
    // Check for ball return
    if ((bx >= ((MAXX - 2) * 16))) {
      if ((by > (aibaty - (8 * 16))) && (by < (aibaty + (8 * 16)))) {
        bvx = -bvx;
      }
      else {
        pscore++;  // AI miss
        winner = PLAYER;
      }
    }
   
    if ((bx <= 0)) {
      if ((by > (baty - (8 * 16))) && (by < (baty + (8 * 16)))) {
        bvx = -bvx;
      }
      else {
        ascore++;  // Player miss
        winner = AI;
      }
    }
    
    // Check for bounces off top/bottom
    if ((by >= ((MAXY - 2) * 16)) || (by <= 0)) {
      bvy = -bvy;
    }
    
    bx += bvx;
    by += bvy;
    
    // Work out timing for this frame
    now = millis ();
    elapsed = now - start;
    
//    Serial.print (elapsed);
//    Serial.println ("ms.");
    
    if (elapsed < 40)
      delay (40 - elapsed);
  }
  
  // Draw final score
  drawBackground ();

  drawBat (LEFTBAT, baty);
  
  drawBat (RIGHTBAT, aibaty);
  
  drawBall (bx, by);

  drawScores (pscore, ascore);
  
  updscreen ();
    
  delay (1000);
  
  return (winner);
}


int airightbat (int baty, const int bx, const int by, const int bvx, const int py)
{
  static int state = SHADOW;
  
  if ((bvx > 0) && (bx > (96 * 16))) {
    baty -= (baty - by) / 12;
  }
  else if (bx < (64 * 16)) {
    switch (state) {
    case STAY:
      break;
    case CENTRE:
      baty -= (baty - (CENY * 16)) / 16;
      break;
    case JIGGLE:
      break;
    case SHADOW:
      baty -= (baty - py) / 16;
      break;
    }
  }
  
  if (baty < (8 * 16))
    baty = 8 * 16;
      
  if (baty > ((MAXY - 8) * 16))
    baty = (MAXY - 8) * 16;
      
  return (baty);
}


/* drawBackground --- draw the screen background */

void drawBackground (void)
{
  int y;
  
  clrFrame ();
  
  for (y = 0; y < MAXY; y += 2) {
    setPixel (CENX, y); // The net
  }
}


/* drawBat --- draw the bat in the framebuffer */

void drawBat (const int x, int y)
{
  y /= 16;
  
  // Bats are two pixels thick for better visibility
  setVline (x, y - 8, y + 8);
  setVline (x + 1, y - 8, y + 8);
}


/* drawBall --- draw the ball in the freamebuffer */

void drawBall (int bx, int by)
{
  bx /= 16;
  by /= 16;
  
  // Ball is 2x2 pixels for better visibility
  setPixel (bx, by);
  setPixel (bx + 1, by);
  setPixel (bx, by + 1);
  setPixel (bx + 1, by + 1);
}


/* drawScores --- draw the scores in the framebuffer */

void drawScores (int left, int right)
{
  char num[4];
  
  snprintf (num, 4, "%d", left);
  setText (32, 0, num);
  
  snprintf (num, 4, "%d", right);
  setText (96, 0, num);
}


/* clrFrame --- clear the entire frame to white */

void clrFrame (void)
{
#ifdef SLOW_CLRFRAME
  int r, c;
  
  for (r = 0; r < MAXROWS; r++) {
    for (c = 0; c < MAXX; c++) {
      Frame[r][c] = 0x00;
    }
  }
#else
  memset (Frame, 0, sizeof (Frame));
#endif
}


/* greyFrame --- clear entire frame to checkerboard pattern */

void greyFrame (void)
{
  int r, c;
  
  for (r = 0; r < MAXROWS; r++) {
    for (c = 0; c < MAXX; c += 2) {
      Frame[r][c] = 0xaa;
      Frame[r][c + 1] = 0x55;
    }
  }
}


/* textRoundRect --- draw a rounded rectangle containing text message */

void textRoundRect (const char *str)
{
  const int len = strlen (str);
  
  fillRoundRect (CENX - (3 * len) - 6, CENY - 4, CENX + (3 * len) + 5, CENY + 10, 7);
  setText (CENX - (3 * len), CENY, str);
}


/* textRoundRect2 --- draw a rounded rectangle containing two text messages */

void textRoundRect2 (const char *str1, const char *str2)
{
  const int len1 = strlen (str1);
  const int len2 = strlen (str2);
  int len;
    
  if (len1 > len2) {
    len = len1;
  }
  else {
    len = len2;
  }
    
  fillRoundRect (CENX - (3 * len) - 6, CENY - 12, CENX + (3 * len) + 5, CENY + 10, 7);
  setText (CENX - (3 * len1), CENY - 8, str1);
  setText (CENX - (3 * len2), CENY, str2);
}


/* setText --- draw text into buffer using predefined font */

void setText (int x, const int y, const char *str)
{
  const int r = y >> 3;
  int i;
  int d;
  
  for ( ; *str; str++) {
    d = (*str - ' ') * 5;
    
    for (i = 0; i < 5; i++) {
      Frame[r][x++] = font_data[d++];
    }
    
    Frame[r][x++] = 0;
  }
}


/* setLine --- draw a line between any two absolute co-ords */

void setLine (int x1, int y1, int x2, int y2)
{
   int dx, dy;
   int d;
   int i1, i2;
   int x, y;
   int xend, yend;
   int temp;
   int yinc, xinc;
   
   dx = abs (x2 - x1);
   dy = abs (y2 - y1);
   
   if (((y1 > y2) && (dx < dy)) || ((x1 > x2) && (dx > dy))) {
      temp = y1;
      y1 = y2;
      y2 = temp;

      temp = x1;
      x1 = x2;
      x2 = temp;
   }
   
   if (dy > dx) {
      d = (2 * dx) - dy;       /* Slope > 1 */
      i1 = 2 * dx;
      i2 = 2 * (dx - dy);
      
      if (y1 > y2) {
         x = x2;
         y = y2;
         yend = y1;
      }
      else {
         x = x1;
         y = y1;
         yend = y2;
      }
      
      if (x1 > x2)
         xinc = -1;
      else
         xinc = 1;
      
      setPixel (x, y);
      
      while (y < yend) {
         y++;     
         if (d < 0)
            d += i1;
         else {
            x += xinc;
            d += i2;
         }
         
         setPixel (x, y);
      }
   }
   else {               
      d = (2 * dy) - dx;  /* Slope < 1 */
      i1 = 2 * dy;
      i2 = 2 * (dy - dx);
      
      if (x1 > x2) {
         x = x2;
         y = y2;
         xend = x1;
      }
      else {
         x = x1;
         y = y1;
         xend = x2;
      }
      
      if (y1 > y2)
         yinc = -1;
      else
         yinc = 1;
      
      setPixel (x, y);
      
      while (x < xend) {
         x++;
         if (d < 0)
            d += i1;
         else {
            y += yinc;
            d += i2;
         }
         
         setPixel (x, y);
      }
   }
}


/* circle --- draw a circle using Michener's algorithm */

void circle (int x0, int y0, int r, int ec, int fc)
{
   int x, y;
   int d;

   x = 0;
   y = r;
   d = 3 - (2 * r);

   while (x < y) {
      cpts (x0, y0, x, y, ec, fc);
      if (d < 0) {
         d += (4 * x) + 6;
      }
      else {
         d += (4 * (x - y)) + 10;
         y--;
      }
      x++;
   }
   
   if (x == y)
      cpts (x0, y0, x, y, ec, fc);
}


static void cpts (int x0, int y0, int x, int y, int ec, int fc)
{
  if (fc == 1) {
    setHline (x0 - x, x0 + x, y0 + y);
    setHline (x0 - x, x0 + x, y0 - y);
    setHline (x0 - y, x0 + y, y0 + x);
    setHline (x0 - y, x0 + y, y0 - x);
  }
  else if (fc == 0) {
    clrHline (x0 - x, x0 + x, y0 + y);
    clrHline (x0 - x, x0 + x, y0 - y);
    clrHline (x0 - y, x0 + y, y0 + x);
    clrHline (x0 - y, x0 + y, y0 - x);
  }

  if (ec) {
    setPixel (x0 + x, y0 + y);
    setPixel (x0 + y, y0 + x);
    setPixel (x0 - x, y0 - y);
    setPixel (x0 - y, y0 - x);
    setPixel (x0 + x, y0 - y);
    setPixel (x0 + y, y0 - x);
    setPixel (x0 - x, y0 + y);
    setPixel (x0 - y, y0 + x);
  }
  else {
    clrPixel (x + x0, y + y0);
    clrPixel (y + x0, x + y0);
    clrPixel (x0 - x, y0 - y);
    clrPixel (x0 - y, y0 - x);
    clrPixel (x + x0, y0 - y);
    clrPixel (y + x0, y0 - x);
    clrPixel (x0 - x, y + y0);
    clrPixel (x0 - y, x + y0);
  }
}

/* drawRoundRect --- draw a rounded rectangle */

void drawRoundRect (int x0, int y0, int x1, int y1, int r)
{
  setHline (x0 + r, x1 - r, y0);
  setHline (x0 + r, x1 - r, y1);
  setVline (x0, y0 + r, y1 - r);
  setVline (x1, y0 + r, y1 - r);
  
  drawSplitCircle (x0 + r, y0 + r, x1 - r, y1 - r, r, 1, -1);
}


/* fillRoundRect --- fill a rounded rectangle */

void fillRoundRect (int x0, int y0, int x1, int y1, int r)
{
  int y;
  
  drawSplitCircle (x0 + r, y0 + r, x1 - r, y1 - r, r, 1, 0);
  
  setHline (x0 + r, x1 - r, y0);
  setHline (x0 + r, x1 - r, y1);
  setVline (x0, y0 + r, y1 - r);
  setVline (x1, y0 + r, y1 - r);
  
  for (y = y0 + r; y < (y1 - r); y++)
    clrHline (x0 + 1, x1 - 1, y);
}


/* drawSplitCircle --- draw a split circle with edge and fill colours */

void drawSplitCircle (int x0, int y0, int x1, int y1, int r, int ec, int fc)
{
  // Michener's circle algorithm. Originally coded on the IBM PC
  // with EGA card in 1986.
  int x, y;
  int d;

  x = 0;
  y = r;
  d = 3 - (2 * r);

  if (fc >= 0) {
    while (x < y) {
      splitcfill (x0, y0, x1, y1, x, y, fc);
      if (d < 0) {
        d += (4 * x) + 6;
      }
      else {
        d += (4 * (x - y)) + 10;
        y--;
      }
      x++;
    }
    
    if (x == y)
      splitcfill (x0, y0, x1, y1, x, y, fc);
  }
  
  x = 0;
  y = r;
  d = 3 - (2 * r);

  while (x < y) {
    splitcpts8 (x0, y0, x1, y1, x, y, ec);
    if (d < 0) {
      d += (4 * x) + 6;
    }
    else {
      d += (4 * (x - y)) + 10;
      y--;
    }
    x++;
  }
  
  if (x == y)
    splitcpts8 (x0, y0, x1, y1, x, y, ec);
}


/* cfill --- draw horizontal lines to fill a circle */

static void splitcfill (int x0, int y0, int x1, int y1, int x, int y, int fc)
{
  if (fc) {
    setHline (x0 - x, x1 + x, y1 + y);
    setHline (x0 - x, x1 + x, y0 - y);
    setHline (x0 - y, x1 + y, y1 + x);
    setHline (x0 - y, x1 + y, y0 - x);
  }
  else {
    clrHline (x0 - x, x1 + x, y1 + y);
    clrHline (x0 - x, x1 + x, y0 - y);
    clrHline (x0 - y, x1 + y, y1 + x);
    clrHline (x0 - y, x1 + y, y0 - x);
  }
}


/* splitcpts8 --- draw eight pixels to form the edge of a split circle */

static void splitcpts8 (int x0, int y0, int x1, int y1, int x, int y, int ec)
{
  splitcpts4 (x0, y0, x1, y1, x, y, ec);

// if (x != y)
    splitcpts4 (x0, y0, x1, y1, y, x, ec);
}


/* splitcpts4 --- draw four pixels to form the edge of a split circle */

static void splitcpts4 (int x0, int y0, int x1, int y1, int x, int y, int ec)
{
  if (ec) {
    setPixel (x1 + x, y1 + y);

//  if (x != 0)
      setPixel (x0 - x, y1 + y);

//  if (y != 0)  
      setPixel (x1 + x, y0 - y);

//  if ((x != 0) && (y != 0))
      setPixel (x0 - x, y0 - y);
  }
  else {
    clrPixel (x1 + x, y1 + y);

//  if (x != 0)
      clrPixel (x0 - x, y1 + y);

//  if (y != 0)
      clrPixel (x1 + x, y0 - y);

//  if ((x != 0) && (y != 0))
      clrPixel (x0 - x, y0 - y);
  }
}


/* setVline --- draw vertical line */

void setVline (unsigned int x, unsigned int y1, unsigned int y2)
{
  unsigned int y;
  
  for (y = y1; y <= y2; y++)
    setPixel (x, y);
}


/* clrVline --- draw vertical line */

void clrVline (unsigned int x, unsigned int y1, unsigned int y2)
{
  unsigned int y;
  
  for (y = y1; y <= y2; y++)
    clrPixel (x, y);
}


//* setHline --- set pixels in a horizontal line */

void setHline (unsigned int x1, unsigned int x2, unsigned int y)
{
  unsigned int x;
  unsigned int row;
  unsigned char b;
  
  row = y / 8;
  b = 1 << (y  & 7);
  
  for (x = x1; x <= x2; x++)
    Frame[row][x] |= b;
}


/* clrHline --- clear pixels in a horizontal line */

void clrHline (unsigned int x1, unsigned int x2, unsigned int y)
{
  unsigned int x;
  unsigned int row;
  unsigned char b;
  
  row = y / 8;
  b = ~(1 << (y  & 7));
  
  for (x = x1; x <= x2; x++)
    Frame[row][x] &= b;
}


/* setRect --- set pixels in a (non-filled) rectangle */

void setRect (int x1, int y1, int x2, int y2)
{
  setHline (x1, x2, y1);
  setVline (x2, y1, y2);
  setHline (x1, x2, y2);
  setVline (x1, y1, y2);
}


/* fillRect --- set pixels in a filled rectangle */

void fillRect (int x1, int y1, int x2, int y2, int ec, int fc)
{
  int y;
  
  for (y = y1; y <= y2; y++)
    if (fc == 0)
      clrHline (x1, x2, y);
    else if (fc == 1)
      setHline (x1, x2, y);
  
  if (ec == 1) {
    setHline (x1, x2, y1);
    setVline (x2, y1, y2);
    setHline (x1, x2, y2);
    setVline (x1, y1, y2);
  }
  else if (ec == 0) {
    clrHline (x1, x2, y1);
    clrVline (x2, y1, y2);
    clrHline (x1, x2, y2);
    clrVline (x1, y1, y2);
  }
}


/* setPixel --- set a single pixel */

void setPixel (unsigned int x, unsigned int y)
{
  if ((x < MAXX) && (y < MAXY))
    Frame[y / 8][x] |= 1 << (y & 7);
  else {
//  Serial.print ("setPixel(");
//  Serial.print (x);
//  Serial.print (",");    
//  Serial.print (y);
//  Serial.println (")");    
  }
}


/* clrPixel --- clear a single pixel */

void clrPixel (unsigned int x, unsigned int y)
{
  if ((x < MAXX) && (y < MAXY))
    Frame[y / 8][x] &= ~(1 << (y & 7));
  else {
//  Serial.print ("clrPixel(");
//  Serial.print (x);
//  Serial.print (",");
//  Serial.print (y);
//  Serial.println (")");    
  }
}


/* updscreen --- update the physical screen from the buffer */

void updscreen (void)
{
  // This function contains an eight-way unrolled loop. In the Arduino
  // IDE, the default GCC optimisation switch is -Os, which optimises
  // for space. No automatic loop unrolling is done by the compiler, so
  // we do it explicitly here to save a few microseconds.
//  long int before, after;
//  unsigned char r, c;
  unsigned char *p;
  int i;
  
  oledCmd (SSD1306_COLUMNADDR);
  oledCmd (0);   // Column start address (0 = reset)
  oledCmd (MAXX - 1); // Column end address (127 = reset)

  oledCmd (SSD1306_PAGEADDR);
  oledCmd (0); // Page start address (0 = reset)
  oledCmd (7); // Page end address
  
//  before = micros ();
  
  p = &Frame[0][0];
  
  for (i = 0; i < ((MAXROWS * MAXX) / 8); i++) {
    oledData (*p++);
    oledData (*p++);
    oledData (*p++);
    oledData (*p++);
    oledData (*p++);
    oledData (*p++);
    oledData (*p++);
    oledData (*p++);
  }

/*
  The slow way...
  for (r = 0; r < MAXROWS; r++) {
    for (c = 0; c < MAXX; c++) {
      oledData (Frame[r][c]);
    }
  }
*/

//  after = micros ();
  
//  Serial.print (after - before);
//  Serial.println ("us updscreen");
}


/* oled1202_begin --- initialise the 128x64 OLED */

void oled_begin (void)
{
  /* Configure I/O pins on Arduino */
  pinMode (slaveSelectPin, OUTPUT);
  pinMode (RSTPin, OUTPUT);
  pinMode (DCPin, OUTPUT);
  pinMode (SDAPin, OUTPUT);
  pinMode (SCLKPin, OUTPUT);
  
  digitalWrite (slaveSelectPin, HIGH);
  digitalWrite (RSTPin, HIGH);
  digitalWrite (DCPin, HIGH);
  digitalWrite (SDAPin, HIGH);
  digitalWrite (SCLKPin, HIGH);

  SPI.begin ();
  // The following line fails on arduino-0021 due to a bug in the SPI library
  // Compile with arduino-0022 or later
  SPI.setClockDivider (SPI_CLOCK_DIV4); // 4MHz
  //SPI.setBitOrder (MSBFIRST);
  //SPI.setDataMode (SPI_MODE3);
  
  /* Start configuring the SSD1306 OLED controller */
  delay (1);
  digitalWrite (RSTPin, LOW); // Hardware reset for 10ms
  delay (10);
  digitalWrite (RSTPin, HIGH);
   
  // Init sequence for 128x64 OLED module
  oledCmd (SSD1306_DISPLAYOFF);                    // 0xAE
  oledCmd (SSD1306_SETDISPLAYCLOCKDIV);            // 0xD5
  oledCmd (0x80);                                  // the suggested ratio 0x80
  oledCmd (SSD1306_SETMULTIPLEX);                  // 0xA8
  oledCmd (0x3F);
  oledCmd (SSD1306_SETDISPLAYOFFSET);              // 0xD3
  oledCmd (0x0);                                   // no offset
  oledCmd (SSD1306_SETSTARTLINE | 0x0);            // line #0
  oledCmd (SSD1306_CHARGEPUMP);                    // 0x8D
  oledCmd (0x14);
  oledCmd (SSD1306_MEMORYMODE);                    // 0x20
  oledCmd (0x00);                                  // 0x0 act like ks0108
  oledCmd (SSD1306_SEGREMAP | 0x1);
  oledCmd (SSD1306_COMSCANDEC);
  oledCmd (SSD1306_SETCOMPINS);                    // 0xDA
  oledCmd (0x12);
  oledCmd (SSD1306_SETCONTRAST);                   // 0x81
  oledCmd (0xCF);
  oledCmd (SSD1306_SETPRECHARGE);                  // 0xd9
  oledCmd (0xF1);
  oledCmd (SSD1306_SETVCOMDETECT);                 // 0xDB
  oledCmd (0x40);
  oledCmd (SSD1306_DISPLAYALLON_RESUME);           // 0xA4
  oledCmd (SSD1306_NORMALDISPLAY);                 // 0xA6
  
  oledCmd (SSD1306_DISPLAYON); // Turn on OLED panel
}


/* oledData --- send a data byte to the OLED by fast hardware SPI */

inline void oledData (unsigned char d)
{
  OLEDOUT |= DC;
  OLEDOUT &= ~CS;
  
  SPCR |= 1 << SPE;
  
  SPDR = d;
  
  while (!(SPSR & (1 << SPIF)))
    ;
    
  SPCR &= ~(1 << SPE);
   
  OLEDOUT |= CS;
}


/* oledCmd --- send a command byte to the OLED by fast hardware SPI */

inline void oledCmd (unsigned char d)
{
  OLEDOUT &= ~DC;
  OLEDOUT &= ~CS;
  
  SPCR |= 1 << SPE;
  
  SPDR = d;
  
  while (!(SPSR & (1 << SPIF)))
    ;
    
  SPCR &= ~(1 << SPE);
   
  OLEDOUT |= CS;
}


