#include <gb/gb.h>
#include <gbdk/font.h>
#include <types.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <rand.h>
#include <hUGEDriver.h>
#include "arrows.h"
#include "CodePieces.h"
#include "TitleScreen.h"
#include "GameplayMap.h"
#include "MenuMap.h"
#include "LoseMap.h"
#include "WinMap.h"

uint8_t current_line = 0;
const uint8_t first_line_offset = 3;
uint8_t max_lines = 7;
uint8_t selected_piece = 0;
uint8_t piece_types = 6;
const uint8_t base_piece_addr = 0x60;
const uint8_t correct_addr = 0x68;
const uint8_t present_addr = 0x69;
const uint8_t answerStartTile = 12; // X tile offset of first answer icon.
uint8_t pieces[4];
uint8_t answer[4];
uint8_t correctCount = 0;
const uint8_t fontTileCount = 96;
uint8_t input = 0;
uint8_t lastInput = 0;
uint8_t selected_difficulty = 0;

extern const hUGESong_t lose;
extern const hUGESong_t winner;

inline void Sound_StartPress(void)
{
    NR10_REG = 0x15;
    NR11_REG = 0x80;
    NR12_REG = 0x63;
    NR13_REG = 0x00;
    NR14_REG = 0xC5;
    NR41_REG = 0x01;
    NR42_REG = 0xF1;
    NR43_REG = 0x00;
    NR44_REG = 0x80;
}

inline void Sound_Scroll(void)
{
    NR10_REG = 0x15;
    NR11_REG = 0xF2;
    NR12_REG = 0xF0;
    NR13_REG = 0x00;
    NR14_REG = 0xC5;
}

inline void Input(void)
{
    while(input == lastInput)
    {
        input = joypad();
        vsync();
    }
    lastInput = input;
}

void EnterCode(void)
{
    // Initialize all piece tiles to piece 0
    memset(pieces, 0, sizeof(pieces));
    for(uint8_t i = 0; i < 4; i++)
    {
        set_bkg_tile_xy((i * 2) + 3, first_line_offset + (current_line * 2), base_piece_addr);
    }

    // Set up selection arrows
    selected_piece = 0;
    set_sprite_tile(0, 0);
    set_sprite_tile(1, 3);
    move_sprite(0, 32, 32 + (current_line * 16));
    move_sprite(1, 32, 48 + (current_line * 16));

    SHOW_SPRITES;

    while(1)
    {
        Input();

        if(input & J_B || input & J_LEFT)
        {
            if(selected_piece > 0)
            {
                scroll_sprite(0, -16, 0);
                scroll_sprite(1, -16, 0);
                selected_piece--;
            }
        }
        else if(input & J_A || input & J_RIGHT)
        {
            if(selected_piece < 3)
            {
                scroll_sprite(0, 16, 0);
                scroll_sprite(1, 16, 0);
                selected_piece++;
            }
        }
        else if(input & J_DOWN)
        {
            if(pieces[selected_piece] > 0)
            {
                pieces[selected_piece]--;
            }
            else
            {
                pieces[selected_piece] = piece_types - 1;
            }
            set_bkg_tile_xy((selected_piece * 2) + 3, first_line_offset + (current_line * 2), pieces[selected_piece] + base_piece_addr);
            Sound_Scroll();
        }
        else if(input & J_UP)
        {
            if(pieces[selected_piece] < piece_types - 1)
            {
                pieces[selected_piece]++;
            }
            else
            {
                pieces[selected_piece] = 0;
            }
            set_bkg_tile_xy((selected_piece * 2) + 3, first_line_offset + (current_line * 2), pieces[selected_piece] + base_piece_addr);
            Sound_Scroll();
        }
        else if(input &  J_START)
        {
            Sound_StartPress();
            delay(100);
            return;
        }

        vsync();
    }
}

int CheckCode(void)
{
    // Loop to check for correct guesses followed by loop to check
    // for pieces that are present but in the wrong spot.
    // Optimize this when I have time.
    uint8_t incorrectSpots[4]; // An array of the values to check in the present check phase.
    correctCount = 0;
    for(uint8_t i = 0; i < 4; i++)
    {
        if(answer[i] == pieces[i])
        {
            incorrectSpots[i] = UINT8_MAX;
            correctCount++;
        }
        else
        {
            incorrectSpots[i] = answer[i];
        }
    }

    uint8_t presentCount = 0;
    for(uint8_t i = 0; i < 4; i++)
    {
        for(uint8_t j = 0; j < 4; j++)
        {
            if(pieces[i] == incorrectSpots[j] && pieces[i] != answer[i])
            {
                // We've counted this piece as present, so remove it from the spots to check.
                incorrectSpots[j] = UINT8_MAX;
                presentCount++;
                break;
            }
        }
    }
    
    for(uint8_t i = 0; i < correctCount; i++)
    {
        // The X offset for the first correct marker is defined as answerStartTile.
        // The X offset for each additional correct marker should be 2 more tiles after that.
        set_bkg_tile_xy(answerStartTile + (i * 2), first_line_offset + (current_line * 2), correct_addr);
    }
    for(uint8_t i = 0; i < presentCount; i++)
    {
        // The X offset for the first present marker should be 2 after the last correct marker.
        // The X offset for each additional correct marker should be 2 more tiles after that.
        set_bkg_tile_xy(answerStartTile + (correctCount * 2) + (i * 2), first_line_offset + (current_line * 2), present_addr);
    }

    return correctCount;
}

void HelpScreen(void)
{
    init_bkg(0); // Clear the background
    printf("In CodeBreaker you\ntry to guess the\ncode that has been\ngenerated.\n\n\n");
    printf("A correct piece in\nthe correct locationis marked with a\n\nA correct piece in\nthe wrong location\nis marked with a\n\n");
    printf("Easy has 4 pieces.\nMedium has 5 pieces.\nHard has 6 pieces.");
    set_bkg_tile_xy(17, 8, correct_addr);
    set_bkg_tile_xy(17, 12, present_addr);

    while(1)
    {
        Input();
        if(input & J_START)
        {
            Sound_StartPress();
            delay(100);
            break;
        }
    }

    HIDE_BKG;
    printf("\n");
    init_bkg(0);
    set_bkg_tiles(0, 0, 20, 18, MenuMap);
    set_sprite_tile(2, 6);
    move_sprite(2, 48, 56 + (selected_difficulty * 16));
    SHOW_BKG;
}

void WinScreen(void)
{
    init_bkg(0);
    set_bkg_tiles(0, 0, 20, 18, WinMap);

    __critical
    {
        hUGE_init(&winner);
        add_VBL(hUGE_dosound);
    }

    delay(2000);

    __critical
    {
        remove_VBL(hUGE_dosound);
    }

    while(1)
    {
        Input();
        if(input & J_START)
        {
            Sound_StartPress();
            delay(100);
            return;
        }
    }
}

void LoseScreen(void)
{
    init_bkg(0);
    set_bkg_tiles(0, 0, 20, 18, LoseMap);

    // Show correct answer
    for(uint8_t i = 0; i < 4; i++)
    {
        set_bkg_tile_xy(8 + i, 9, answer[i] + base_piece_addr);
    }

    __critical
    {
        hUGE_init(&lose);
        add_VBL(hUGE_dosound);
    }

    delay(2000);

    __critical
    {
        remove_VBL(hUGE_dosound);
    }

    while(1)
    {
        Input();
        if(input & J_START)
        {
            Sound_StartPress();
            delay(100);
            return;
        }
    }
}

void MainMenu(void)
{
    set_bkg_tiles(0, 0, 20, 18, MenuMap);
    set_sprite_tile(2, 6);
    move_sprite(2, 48, 56 + (selected_difficulty * 16));
    SHOW_SPRITES;

    while(1)
    {
        Input();

        if(input & J_START)
        {
            Sound_StartPress();
            delay(100);
            break;
        }
        else if(input & J_DOWN)
        {
            if(selected_difficulty < 2)
            {
                selected_difficulty++;
                scroll_sprite(2, 0, 16);
                Sound_Scroll();
                delay(100);
            }
        }
        else if(input & J_UP)
        {
            if(selected_difficulty > 0)
            {
                selected_difficulty--;
                scroll_sprite(2, 0, -16);
                Sound_Scroll();
                delay(100);
            }
        }
        else if(input & J_SELECT)
        {
            move_sprite(2, 0, 0);
            HelpScreen();
            move_sprite(2, 48, 56 + (selected_difficulty * 16));
        }
        vsync();
    }

    if(selected_difficulty == 0)
    {
        piece_types = 4;
    }
    else if(selected_difficulty == 1)
    {
        piece_types = 5;
    }
    else
    {
        piece_types = 6;
    }
    move_sprite(2, 0, 0);
}

void main(void)
{
    DISPLAY_ON;

    // Title screen
    set_bkg_data(0, TitleScreen_TILE_COUNT, TitleScreen_tiles);
    set_bkg_tiles(0, 0, 20, 18, TitleScreen_map);
    SHOW_BKG;

    NR52_REG = 0x80; // Enable sound
    NR51_REG = 0xFF; // Enable all channels
    NR50_REG = 0x77; // Set left/right vol to max

    while(1)
    {
        if(joypad() & J_START)
        {
            Sound_StartPress();
            delay(100);
            break;
        }
    }

    uint16_t seed = LY_REG;
    seed |= (uint16_t)DIV_REG << 8;
    initrand(seed);

    set_sprite_data(0, 8, Arrows);

    // Loop forever
    while(1)
    {
        // Set up tile data
        init_bkg(0); // Clear the background
        set_bkg_data(fontTileCount, 10, CodePieces);
        font_init();
        font_set(font_load(font_spect));

        MainMenu();

        // Generate a code with random pieces
        for(uint8_t i = 0; i < 4; i++)
        {
            answer[i] = rand() % piece_types;
        }

        set_bkg_tiles(0, 0, 20, 18, GameplayMap); // Set static display elements.
        
        // Loop until the game is won
        while(correctCount != 4 && current_line < max_lines) 
        {
            EnterCode();
            CheckCode();
            current_line++;

            // Done processing, yield CPU and wait for start of next frame
            vsync();
        }

        // Move selection arrow sprites off screen.
        move_sprite(0, 0, 0);
        move_sprite(1, 0, 0);

        // Display win or loss screens
        if(correctCount < 4)
        {
            LoseScreen();
        }
        else
        {
            WinScreen();
        }
        
        // Reset game state
        correctCount = 0;
        current_line = 0;
    }
}