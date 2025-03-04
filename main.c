#include <gb/gb.h>
#include <gbdk/font.h>
#include <types.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <rand.h>
#include "arrows.h"
#include "CodePieces.h"
#include "TitleScreen.h"
#include "GameplayMap.h"

uint8_t current_line = 0;
const uint8_t first_line_offset = 3;
uint8_t max_lines = 6;
uint8_t selected_piece = 0;
uint8_t piece_types = 6;
const uint16_t base_piece_addr = 352;
const uint16_t correct_addr = 360;
const uint16_t present_addr = 361;
const uint8_t answerStartTile = 12; // X tile offset of first answer icon.
uint8_t pieces[4];
uint8_t answer[4];
uint8_t correctCount = 0;

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
    set_sprite_data(0, 6, Arrows);
    set_sprite_tile(0, 0);
    set_sprite_tile(1, 3);
    move_sprite(0, 32, 32 + (current_line * 16));
    move_sprite(1, 32, 48 + (current_line * 16));

    SHOW_SPRITES;

    while(1)
    {
        switch(joypad())
        {
            case J_B:
                if(selected_piece > 0)
                {
                    scroll_sprite(0, -16, 0);
                    scroll_sprite(1, -16, 0);
                    selected_piece--;
                }
                delay(100);
                break;
            case J_A:
                if(selected_piece < 3)
                {
                    scroll_sprite(0, 16, 0);
                    scroll_sprite(1, 16, 0);
                    selected_piece++;
                }
                delay(100);
                break;
            case J_DOWN:
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
                break;
            case J_UP:
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
                break;
            case J_START:
                delay(100);
                return;
            default:
                break;
        }
        // Debounce input
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

void main(void)
{
    DISPLAY_ON;

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

    // Loop forever
    while(1)
    {
        // Generate a code with random pieces
        for(uint8_t i = 0; i < 4; i++)
        {
            answer[i] = rand() % piece_types;
        }

        // Set up tile data
        init_bkg(256); // Clear the background
        set_bkg_data(96, 10, CodePieces);
        font_init();
        font_set(font_load(font_spect));
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
        // Reset the game state.
        correctCount = 0;
        current_line = 0;
    }
}