/* Standard library */
#include <stdint.h>

/* Local headers */
#include <graphics.h>
#include <fonts.h>
#include <lib.h>

// Retrieved from https://wiki.osdev.org/VESA_Video_Modes
struct vbe_mode_info_structure {
    uint16_t attributes;
    uint8_t window_a;
    uint8_t window_b;
    uint16_t granularity;
    uint16_t window_size;
    uint16_t segment_a;
    uint16_t segment_b;
    uint32_t win_func_ptr;
    uint16_t pitch;
    uint16_t width;
    uint16_t height;
    uint8_t w_char;
    uint8_t y_char;
    uint8_t planes;
    uint8_t bpp;
    uint8_t banks;
    uint8_t memory_model;
    uint8_t bank_size; 
    uint8_t image_pages;
    uint8_t reserved0;

    uint8_t red_mask;
    uint8_t red_position;
    uint8_t green_mask;
    uint8_t green_position;
    uint8_t blue_mask;
    uint8_t blue_position;
    uint8_t reserved_mask;
    uint8_t reserved_position;
    uint8_t direct_color_attributes;

    uint32_t framebuffer;
    uint32_t off_screen_mem_off;
    uint16_t off_screen_mem_size;
    uint8_t reserved1[206];
} __attribute__((packed));

/**
 * @struct screen
 * @brief represents the shell screen
 * 
 * @note this struct allows scaling from a single screen to multiple screens
 */
typedef struct {
    uint8_t current_i;      /// < Current i pixel
    uint8_t current_j;      /// < Current j pixel
    uint8_t start_i;        /// < Screen start i pixel
    uint8_t start_j;        /// < Screen start j pixel
    uint16_t width, height; /// < Screen dimensions
} TScreen;

const TColor RED = {0xFF, 0x00, 0x00};
const TColor WHITE = {0xFF, 0xFF, 0xFF};
const TColor BLACK = {0x00, 0x00, 0x00};

static TScreen screen;

static char buffer[64] = {'0'};
static const struct vbe_mode_info_structure* graphicModeInfo = (struct vbe_mode_info_structure*)0x5C00;

static void getNextPosition();
static void checkSpace();
static void scrollUp();

static void* getPixelAddress(int i, int j) {
    return (void*)((uint64_t)graphicModeInfo->framebuffer + 3 * (graphicModeInfo->width * i + j));
}

static void drawPixel(int i, int j, const TColor* color) {
    uint8_t* pixel = getPixelAddress(i, j);
    pixel[0] = color->B;
    pixel[1] = color->G;
    pixel[2] = color->R;
}

// Default screen
void scr_init() {
    screen.current_i = 0;
    screen.current_j = 0;
    screen.start_i = 0;
    screen.start_j = 0;
    screen.width = graphicModeInfo->width / CHAR_WIDTH;
    screen.height = graphicModeInfo->height / CHAR_HEIGHT;
    scr_clear();
}

void scr_printCharFormat(char c, const TColor* charColor, const TColor* bgColor) {

    // Backspace
    if (c == '\b') {
        if (screen.current_j == 0) {
            screen.current_i -= 1;
            screen.current_j = screen.width - 1;
            scr_printCharFormat(' ', charColor, bgColor);
            screen.current_i -= 1;
            screen.current_j = screen.width - 1;
        } else {
            screen.current_j = (screen.current_j - 1) % screen.width;
            scr_printCharFormat(' ', charColor, bgColor);
            screen.current_j = (screen.current_j - 1) % screen.width;
        }
        return;
    }

    checkSpace();

    // scr_printLine
    if (c == '\n') {
        scr_printLine();
        return;
    }

    uint8_t* character = getCharMapping(c);

    // Upper left pixel of the current character
    uint16_t write_i = (screen.start_i + screen.current_i) * CHAR_HEIGHT;
    uint16_t write_j = (screen.start_j + screen.current_j) * CHAR_WIDTH;

    uint8_t mask;

    for (int i = 0; i < CHAR_HEIGHT; ++i) {
        for (int j = 0; j < CHAR_WIDTH; ++j) {
            mask = 1 << (CHAR_WIDTH - j - 1);
            if (character[i] & mask) {
                drawPixel(write_i + i, write_j + j, charColor);
            } else {
                drawPixel(write_i + i, write_j + j, bgColor);
            }
        }
    }
    getNextPosition();
}

void scr_printChar(char c) {
    scr_printCharFormat(c, &WHITE, &BLACK);
}

void scr_print(const char* string) {
    for (int i = 0; string[i] != 0; ++i) {
        scr_printChar(string[i]);
    }
}

void scr_printLine() {
    screen.current_j = 0;
    screen.current_i += 1;
}

void scr_restartCursor() {
    screen.current_i = 0;
    screen.current_j = 0;
}

void scr_clear() {
    screen.current_i = 0;
    screen.current_j = 0;
    for (int i = 0; i < screen.height; ++i) {
        for (int j = 0; j < screen.width; ++j) {
            scr_printCharFormat(' ', &WHITE, &BLACK);
        }
    }
    screen.current_i = 0;
    screen.current_j = 0;
}

void scr_printDec(uint64_t value) {
    scr_printBase(value, 10);
}

void scr_printHex(uint64_t value) {
    scr_printBase(value, 16);
}

void scr_printBin(uint64_t value) {
    scr_printBase(value, 2);
}

void scr_printBase(uint64_t value, uint32_t base) {
    uintToBase(value, buffer, base);
    scr_print(buffer);
}

// Function to print in register format
void scr_printRegisterFormat(uint64_t reg) {

    uint64_t aux = reg;
    uint64_t count = 16;

    while (aux) {
        aux = aux >> 4;
        --count;
    }

    for (int i = 0; i < count; i++) {
        scr_printChar('0');
    }

    if (reg) {
        scr_printHex(reg);
    }
}

static void getNextPosition() {
    screen.current_i += ((screen.current_j + 1) == screen.width) ? 1 : 0;
    screen.current_j = (screen.current_j + 1) % screen.width;
}

static void checkSpace() {
    if (screen.current_i == screen.height) {
        scrollUp();
    }
}

static void scrollUp() {
    for (int i = 1; i < screen.height * CHAR_HEIGHT; ++i) {

        uint8_t* start = getPixelAddress(screen.start_i + i, screen.start_j);
        uint8_t* next = getPixelAddress(screen.start_i + CHAR_HEIGHT + i, screen.start_j);

        for (int j = 0; j < screen.width * CHAR_WIDTH * 3; ++j) {
            start[j] = next[j];
        }
    }
    screen.current_i -= 1;
}