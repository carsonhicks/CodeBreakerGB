#include <gb/gb.h>
#include <gbdk/font.h>
#include <types.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <rand.h>
#include "arrows.h"
#include "CodePieces.h"
#include "TitleScreen.h"
#include "GameplayMap.h"
#include "MenuMap.h"

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
bool startDebounce = false;
uint8_t input = 0;
uint8_t selected_difficulty = 0;

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
        input = joypad();

        if(!startDebounce && (input ^ J_START))
        {
            startDebounce = true;
        }

        if(input & J_B || input & J_LEFT)
        {
            if(selected_piece > 0)
            {
                scroll_sprite(0, -16, 0);
                scroll_sprite(1, -16, 0);
                selected_piece--;
            }
            delay(100);
        }
        else if(input & J_A || input & J_RIGHT)
        {
            if(selected_piece < 3)
            {
                scroll_sprite(0, 16, 0);
                scroll_sprite(1, 16, 0);
                selected_piece++;
            }
            delay(100);
        }

        if(input & J_DOWN)
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
            delay(100);
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
            delay(100);
        }

        if(startDebounce && (input & J_START))
        {
            startDebounce = false;
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
            if(pieces[i] == incorrectSpots[j])
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

void MainMenu(void)
{
    set_bkg_tiles(0, 0, 20, 18, MenuMap);
    set_sprite_tile(2, 6);
    move_sprite(2, 48, 56 + (selected_difficulty * 16));
    SHOW_SPRITES;

    while(1)
    {
        input = joypad();

        if(!startDebounce && (input ^ J_START))
        {
            startDebounce = true;
        }

        if(startDebounce && (input & J_START))
        {
            startDebounce = false;
            delay(100);
            break;
        }
        else if(input & J_DOWN)
        {
            if(selected_difficulty < 2)
            {
                selected_difficulty++;
                scroll_sprite(2, 0, 16);
                delay(100);
            }
        }
        else if(input & J_UP)
        {
            if(selected_difficulty > 0)
            {
                selected_difficulty--;
                scroll_sprite(2, 0, -16);
                delay(100);
            }
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

    while(1)
    {
        if(joypad() & J_START)
        {
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

        // Reset the game state and move selection arrow sprites off screen.
        correctCount = 0;
        current_line = 0;
        move_sprite(0, 0, 0);
        move_sprite(1, 0, 0);
    }
}