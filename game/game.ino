#include "Arduboy.h"

// global Arduboy instance

Arduboy arduboy;

// define frame rate

#define FRAME_RATE 24

// define draw color

#define COLOR WHITE

// declare program variables

int button_pressed = 0; // checking if button has been pressed in order to create delay
uint8_t last_button = 0;    // marks which button was pressed last: 1=right, 2=left, 3=up, 4=down


// Frogger bitmap

  const uint8_t PROGMEM frogger_bitmap[] = {0x46, 0x20, 0xCF, 0x30, 0x56, 0xA0, 0x7F, 0xE0, 
  0x1F, 0x80, 0x1F, 0x80, 0x17, 0x80, 0x77, 0xE0, 
  0x5B, 0xA0, 0x4F, 0x20, 0xE6, 0x70, 0x40, 0x20, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// struct definition for frogger

typedef struct frogger_t {

  int x;
  int y;
  int h = 12;
  int w = 12;
  int row;                                       // which row Frogger is in
  const uint8_t PROGMEM *bitmap = frogger_bitmap;
} Frogger;

// struct definition for obstacles
// acts as linked list node
typedef struct obstacle_t {

  int x_min;               // x coordinate of left of obstacle
  int w;                   // width of the obstacle
  struct obstacle_t *next; // pointer to next obstacle
} Obstacle;

// struct definition for obstacle row
// acts as linked list
typedef struct obstacle_row_t {

  int y;          // position on screen
  int row_speed;  // positive or negative indicates direction
  int bitmap;     // index of bitmap in bitmap array
  Obstacle *head; // pointer to head obstacle (closest to going offscreen)
  Obstacle *tail; // pointer to tail obstacle (furthest from going off screen)
} Row;

  // racecar bitmap

  const uint8_t PROGMEM racecar_bitmap[] = {0x7C, 0x00, 0x00, 0x7C, 0x38, 0x00, 0x10, 0x10, 
0x00, 0xFF, 0xFC, 0x00, 0x45, 0x8F, 0x00, 0x22, 
0xC3, 0xC0, 0x22, 0xC3, 0xC0, 0x45, 0x8F, 0x00, 
0xFF, 0xFC, 0x00, 0x10, 0x10, 0x00, 0x7C, 0x38, 
0x00, 0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  // van bitmap

  const uint8_t PROGMEM van_bitmap[] = {0x1E, 0x1C, 0x00, 0x7F, 0x7F, 0x00, 0xB1, 0xF1, 
  0xC0, 0xE1, 0xE1, 0xC0, 0xE3, 0xE1, 0x80, 0xE1, 
  0xE1, 0x80, 0xE1, 0xE1, 0x80, 0xE3, 0xE1, 0x80, 
  0xE1, 0xE1, 0xC0, 0xB1, 0xF1, 0xC0, 0x7F, 0x7F, 
  0x00, 0x1E, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  // short_truck bitmap

  const uint8_t PROGMEM short_truck_bitmap[] = {0x38, 0x0E, 0x00, 0xFF, 0xFF, 0x80, 0x80, 0x00, 
  0xF8, 0x80, 0x00, 0xCC, 0xFF, 0xFF, 0xD3, 0x80, 
  0x00, 0xD3, 0x80, 0x00, 0xD3, 0xFF, 0xFF, 0xD3, 
  0x80, 0x00, 0xCC, 0x80, 0x00, 0xF8, 0xFF, 0xFF, 
  0x80, 0x38, 0x0E, 0x00};

  // long_truck bitmap

  const uint8_t PROGMEM long_truck_bitmap[] = {0x3B, 0x80, 0x01, 0xC0, 0x00, 0xFF, 0xFF, 0xFF, 
  0xE0, 0x00, 0x80, 0x00, 0x00, 0x27, 0x80, 0x80, 
  0x00, 0x00, 0x3C, 0xC0, 0xFF, 0xFF, 0xFF, 0xFD, 
  0x30, 0x80, 0x00, 0x00, 0x25, 0x30, 0x80, 0x00, 
  0x00, 0x25, 0x30, 0xFF, 0xFF, 0xFF, 0xFD, 0x30, 
  0x80, 0x00, 0x00, 0x3C, 0xC0, 0x80, 0x00, 0x00, 
  0x27, 0x80, 0xFF, 0xFF, 0xFF, 0xE0, 0x00, 0x3B, 
  0x80, 0x01, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00};

// array of bitmap types
const uint8_t PROGMEM * const bitmaps[] = {racecar_bitmap, van_bitmap, short_truck_bitmap, long_truck_bitmap};

// instantiate Frogger
Frogger frogger;

// create row of racecars
Obstacle racecar1{0, 18, NULL};
Obstacle racecar2{32, 18, &racecar1};
Obstacle racecar3{80, 18, &racecar2};
Row racecar_row{40, 1, 0, &racecar3, &racecar1};

void loop_row(Row *r) {
  r->tail->next = r->head;
  r->head = r->head->next;
  r->tail = r->tail->next;
  r->tail->next = NULL; 
}

void move_obstacles(Row *r) {
  Obstacle *curr = r->head;
  while(curr) {
    curr->x_min += r->row_speed;
    curr = curr->next;
  }
  if(r->row_speed < 0) {
    if(r->head->x_min + r->head->w < 0) {
      r->head->x_min = 128;
      loop_row(r);
    }
  }
  else if(r->row_speed > 0) {
    if(r->head->x_min > WIDTH) {
      r->head->x_min = -r->head->w;
      loop_row(r);
    }
  }
}

void detect_collisions(Row r, Frogger *frogger) {

  Obstacle * curr = r.head;

  while(curr) {
    if ((curr->x_min < frogger->x && (curr->x_min + curr->w) > frogger->x) || (curr->x_min < (frogger->x + frogger->w) && (curr->x_min + curr->w) > (frogger->x + frogger->w))) {
        // set Frogger initial position
        frogger->x = WIDTH/2;
        frogger->y = HEIGHT-frogger->h;
        frogger->row = 0;  
    }
    curr = curr->next;
  }
  
}

void setup() {

  // initialize arduboy

  arduboy.begin();

  // set framerate

  arduboy.setFrameRate(FRAME_RATE);

  // set Frogger initial position
  frogger.x = WIDTH/2;
  frogger.y = HEIGHT-frogger.h;
  frogger.row = 0;
}

void loop() {

  if (!arduboy.nextFrame()) return;

  if(button_pressed) {
    if(arduboy.notPressed(last_button)) {
      button_pressed = 0;
    }
  }
  else {
  
    // move 1 pixel to the right if the right button is pressed
  
    if(arduboy.pressed(RIGHT_BUTTON) && (frogger.x < WIDTH - 2*frogger.w)) {
  
      frogger.x += 12;
      button_pressed = 1;
      last_button = RIGHT_BUTTON;
    }
  
    // move 1 pixel to the left if the left button is pressed
  
    if(arduboy.pressed(LEFT_BUTTON) && (frogger.x > frogger.w)) {
  
      frogger.x -= 12;
      button_pressed = 1;
      last_button = LEFT_BUTTON;
    }
  
    // move 1 pixel up if the up button is pressed
  
    if(arduboy.pressed(UP_BUTTON) && (frogger.y > frogger.h)) {

      frogger.row += 1;
      frogger.y -= 12;
      button_pressed = 1;
      last_button = UP_BUTTON;
    }
  
    // move 1 pixel down if the down button is pressed
  
    if(arduboy.pressed(DOWN_BUTTON) && (frogger.y < HEIGHT - frogger.h)) {

      frogger.row -= 1;
      frogger.y += 12;
      button_pressed = 1;
      last_button = DOWN_BUTTON;
    }
  }

  // move racecar row
  if(arduboy.everyXFrames(2)) {
    move_obstacles(&racecar_row);
    if (frogger.row == 1)
      detect_collisions(racecar_row, &frogger);
  }
  
  // clear screen

  arduboy.clear();

  // reset x and y

  arduboy.setCursor(frogger.x, frogger.y);

  // draw frogger bitmap

  arduboy.drawSlowXYBitmap(frogger.x, frogger.y, frogger.bitmap, 12, 12, COLOR);

  // draw row

  Obstacle *curr = racecar_row.head;
  while(curr) {
    arduboy.drawSlowXYBitmap(curr->x_min, racecar_row.y, racecar_bitmap, 18, 12, COLOR);
    curr = curr->next;
  }

  // display buffer items on screen

  arduboy.display();
}

