#include <gbdk/platform.h>
#include <stdint.h>
#include <string.h>
#include "Font.h"
#include "DialogueBox.h"

#define DIALOG_BOX_HEIGHT 5
#define BYTES_PER_TILE  1

#define TILE_SIZE_BYTES  (BYTES_PER_TILE*16)
#define INNER_DIALOGUE_BOX_WIDTH (DEVICE_SCREEN_WIDTH-2)

int16_t windowYPosition=0;
uint8_t fontTilesStart=0;

uint8_t joypadCurrent=0,joypadPrevious=0;

uint8_t loadedCharacters[Font_TILE_COUNT];
uint8_t loadedCharacterCount=0;

void MoveWindow(void){

    int16_t y = windowYPosition>>3;

    move_win(7,y);
}


uint8_t GetTileForCharacter(char character){
    
    uint8_t vramTile=0;

    // Char's can be interpreted as integers
    // We don't need to map every alpha-numeric character
    // We can use basic math to simplify A-Z and 0-9
    if(character>='a'&&character<='z')vramTile = (character-'a')+1;
    else if(character>='A'&&character<='Z')vramTile = (character-'A')+1;
    else if(character>='0'&&character<='9')vramTile = (character-'0')+27;
    else if(character=='!')vramTile = 37;
    else if(character==':')vramTile = 38;
    else if(character=='?')vramTile = 39;
    else if(character=='/')vramTile = 40;
    else if(character=='=')vramTile = 41;
    else if(character==',')vramTile = 42;
    else if(character=='.')vramTile = 43;
    else if(character=='<')vramTile = 44;
    else if(character=='>')vramTile = 45;

    return vramTile;
}

uint8_t IsAlphaNumeric(char character){

    // Return true for a-z,A-Z, and 0-9
    if(character>='a'&&character<='z')return TRUE;
    else if(character>='A'&&character<='Z')return TRUE;
    else if(character>='0'&&character<='9')return TRUE;

    return FALSE;
}

uint8_t BreakLineEarly(uint16_t index, uint8_t rowSize, char* text){

    char character = text[index++];

    // We can break, if we are at the end of our row
    if(rowSize>=INNER_DIALOGUE_BOX_WIDTH)return TRUE;

    // We DO NOT  break on alpha-numeric characters
    if(IsAlphaNumeric(character))return FALSE;

    uint8_t nextRow=rowSize+1;

    // Loop ahead until we reach the end of the string
    while((character=text[index++])!='\0'){

        // Stop when we reach a non alphanumeric character
        if(!IsAlphaNumeric(character))break;

        // Increase how many characters we've skipped
        nextRow++;
    }

    // Return TRUE if the distance to the next non alphanumeric character, is larger than we have left on the line
    return nextRow>INNER_DIALOGUE_BOX_WIDTH;
}   


void WaitForAButton(void){
    while(TRUE){

        joypadPrevious=joypadCurrent;
        joypadCurrent = joypad();
        
        if((joypadCurrent & J_A) && !(joypadPrevious & J_A))break;
    }

}

void DrawDialogueBoxOnWin(void){

    
    set_win_based_tiles(0,0,20,5,DialogueBox_map,1);
}

void SlideDialogueBoxOnScreen(void){

    DrawDialogueBoxOnWin();

    int16_t desiredWindowPosition = (DEVICE_SCREEN_HEIGHT<<3)-(DIALOG_BOX_HEIGHT*8);

    while((windowYPosition>>3)>desiredWindowPosition){

        windowYPosition-=10;
        MoveWindow();
        vsync();
    }
}

void SlideDialogueBoxOffScreen(void){

    int16_t desiredWindowPosition = (DEVICE_SCREEN_HEIGHT<<3);

    while((windowYPosition>>3)<desiredWindowPosition){

        windowYPosition+=10;
        MoveWindow();
        vsync();
    }
}

void ResetLoadedCharacters(void){

    loadedCharacterCount=1;

    // Reset everything to 255
    for(uint8_t i=0;i<45;i++)loadedCharacters[i]=255;
}

void vsyncMultiple(uint8_t count){
     // Wait some frames
    // This creats a typewriter effect
    for(uint8_t i=0;i<count;i++){

        vsync();
    }
}

void ShiftTextRowsUpward(void){
    
    uint8_t copyBuffer[INNER_DIALOGUE_BOX_WIDTH];

    // Wait a little bit
    vsyncMultiple(15);

    get_win_tiles(1,3,INNER_DIALOGUE_BOX_WIDTH,1,copyBuffer);

    // Clear the inner dialogue box
    fill_win_rect(1,1,INNER_DIALOGUE_BOX_WIDTH,3,0);

    // Draw the line of text one tile up
    set_win_tiles(1,2,INNER_DIALOGUE_BOX_WIDTH,1,copyBuffer);

    // Wait a little bit
    vsyncMultiple(15);

    // Clear the previous line of text
    fill_win_rect(1,2,INNER_DIALOGUE_BOX_WIDTH,1,0);

    // Draw on the first row of our dialogue box
    set_win_tiles(1,1,INNER_DIALOGUE_BOX_WIDTH,1,copyBuffer);
}

void DrawTextAdvanced(char* text){

    uint8_t column=1;
    uint8_t row=1;
        
    SlideDialogueBoxOnScreen();
    ResetLoadedCharacters();

    uint16_t index=0;
    uint8_t columnSize=0;
    uint8_t rowCount=0;


    // Get the address of the first tile in the row
    uint8_t* vramAddress = get_win_xy_addr(column,row);

    char c;

    while((c=text[index])!='\0'){

        uint8_t vramTile = GetTileForCharacter(c);

        // If we haven't loaded this character into VRAM
        if(loadedCharacters[vramTile]==255){

            // Save where we place this character in VRAM
            loadedCharacters[vramTile]=fontTilesStart+loadedCharacterCount++;

            // Place this character in VRAM
            set_bkg_data(loadedCharacters[vramTile],1,Font_tiles+vramTile*TILE_SIZE_BYTES);

        }

        // Draw our character at the address
        // THEN, increment the address
        set_vram_byte(vramAddress++,loadedCharacters[vramTile]);

        index++;
        columnSize++;

        // If we just drew a period or question mark,
        // wait for the a button  and afterwards clear the dialogue box.
        if( c=='.'||c=='?'){
            
            WaitForAButton();
            DrawDialogueBoxOnWin();
            ResetLoadedCharacters();
            
            rowCount=0;
            columnSize=0;
            
            // reset for the next row
            vramAddress = get_win_xy_addr(column,row+rowCount);

            // If we just started a new line, skip spaces
            while(text[index]==' '){
                index++;
            }
        }

        // if we've reached the end of the row
        else if(BreakLineEarly(index,columnSize,text)){

            rowCount+=2;
            columnSize=0;

            // if we've drawn our 2 rows
            if( rowCount>2){
                ShiftTextRowsUpward();
                rowCount=2;
            }
            
            // reset for the next row
            vramAddress = get_win_xy_addr(column,row+rowCount);

            // If we just started a new line, skip spaces
            while(text[index]==' '){
                index++;
            }
        }

        // Play a basic sound effect
        NR10_REG = 0x34;
        NR11_REG = 0x81;
        NR12_REG = 0x41;
        NR13_REG = 0x7F;
        NR14_REG = 0x86;

        vsyncMultiple(3);
    }

    SlideDialogueBoxOffScreen();
    
}



void ClearScreen(void){
    fill_win_rect(0,0,DEVICE_SCREEN_WIDTH,DEVICE_SCREEN_HEIGHT,0);
    fill_bkg_rect(0,0,DEVICE_SCREEN_WIDTH,DEVICE_SCREEN_HEIGHT,0);
}

void main(void)
{
    DISPLAY_ON;
    SHOW_BKG;
    SHOW_WIN;

    fontTilesStart = DialogueBox_TILE_COUNT+1;
    uint8_t emptyTile[TILE_SIZE_BYTES];
    for(uint8_t i=0;i<TILE_SIZE_BYTES;i++)emptyTile[i]=0;

    set_bkg_data(0,1,emptyTile);
    ClearScreen();

    set_bkg_data(1,DialogueBox_TILE_COUNT,DialogueBox_tiles);
    DrawDialogueBoxOnWin();

    NR52_REG = 0x80; // Master sound on
    NR50_REG = 0xFF; // Maximum volume for left/right speakers. 
    NR51_REG = 0xFF; // Turn on sound fully

    // Completely hide the window
    windowYPosition = (DEVICE_SCREEN_HEIGHT << 3)<<3;
    MoveWindow();
    
    while(TRUE){

        // We'll pass in one long string, but the game will present to the player multiple pages.
        // By passing 3 as the final argument, the game boy will wait 3 frames between each character
        DrawTextAdvanced("This is an how you draw text on the screen in GBDK. The code will automatically jump to a new line, when it cannot fully draw a word.  When you reach three lines, it will wait until you press A. After that, it will start a new page and continue. The code will also automatically start a new page after periods. For every page, the code will dynamically populate VRAM. Only letters and characters used, will be loaded into VRAM.");
        
    }
}
