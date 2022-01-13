// based on Schnappy 2020 hello world : https://github.com/ABelliqueux/nolibgs_hello_worlds/tree/main/hello_world
// Based on Lameguy64 tutorial : http://lameguy64.net/svn/pstutorials/chapter1/1-display.html
// sha256 implementation based on Joseph Matheney https://pastebin.com/EXDsRbYH
#include <sys/types.h>
#include <stdio.h>
#include <libgte.h>
#include <libetc.h>
#include <libgpu.h>
#include <memory.h>
#include <string.h>
// #include <openssl/sha.h>
#include <stdlib.h>
#include "./sha256.c"

#define VMODE 0                 // Video Mode : 0 : NTSC, 1: PAL
#define SCREENXRES 320          // Screen width
#define SCREENYRES 240          // Screen height
#define CENTERX SCREENXRES/2    // Center of screen on x 
#define CENTERY SCREENYRES/2    // Center of screen on y
#define MARGINX 0                // margins for text display
#define MARGINY 32
#define FONTSIZE 8 * 7          // Text Field Height
DISPENV disp[2];                 // Double buffered DISPENV and DRAWENV
DRAWENV draw[2];
short db = 0;                      // index of which buffer is used, values 0, 1
void init(void)
{
    ResetGraph(0);                 // Initialize drawing engine with a complete reset (0)
    SetDefDispEnv(&disp[0], 0, 0         , SCREENXRES, SCREENYRES);     // Set display area for both &disp[0] and &disp[1]
    SetDefDispEnv(&disp[1], 0, SCREENYRES, SCREENXRES, SCREENYRES);     // &disp[0] is on top  of &disp[1]
    SetDefDrawEnv(&draw[0], 0, SCREENYRES, SCREENXRES, SCREENYRES);     // Set draw for both &draw[0] and &draw[1]
    SetDefDrawEnv(&draw[1], 0, 0         , SCREENXRES, SCREENYRES);     // &draw[0] is below &draw[1]
    if (VMODE)                  // PAL
    {
        SetVideoMode(MODE_PAL);
        disp[0].screen.y += 8;  // add offset : 240 + 8 + 8 = 256
        disp[1].screen.y += 8;
        }
    SetDispMask(1);                 // Display on screen    
    setRGB0(&draw[0], 50, 50, 50); // set color for first draw area
    setRGB0(&draw[1], 50, 50, 50); // set color for second draw area
    draw[0].isbg = 1;               // set mask for draw areas. 1 means repainting the area with the RGB color each frame 
    draw[1].isbg = 1;
    PutDispEnv(&disp[db]);          // set the disp and draw environnments
    PutDrawEnv(&draw[db]);
    FntLoad(960, 0);                // Load font to vram at 960,0(+128)
    FntOpen(0, 20, 320, 240, 0, 512); // FntOpen(x, y, width, height,  black_bg, max. nbr. chars
}
void display(void)
{
    DrawSync(0);                    // Wait for all drawing to terminate
    VSync(0);                       // Wait for the next vertical blank
    PutDispEnv(&disp[db]);          // set alternate disp and draw environnments
    PutDrawEnv(&draw[db]);  
    db = !db;                       // flip db value (0 or 1)
}
// this is the block header, it is 80 bytes long (steal this code)
typedef struct block_header {
	unsigned int	version;
	// dont let the "char" fool you, this is binary data not the human readable version
	unsigned char	prev_block[32];
	unsigned char	merkle_root[32];
	unsigned int	timestamp;
	unsigned int	bits;
	unsigned int	nonce;
} block_header;


// we need a helper function to convert hex to binary, this function is unsafe and slow, but very readable (write something better)
void hex2bin(unsigned char* dest, unsigned char* src)
{
	unsigned char bin;
	int c, pos;
	char buf[3];

	pos=0;
	c=0;
	buf[2] = 0;
	while(c < strlen(src))
	{
		// read in 2 characaters at a time
		buf[0] = src[c++];
		buf[1] = src[c++];
		// convert them to a interger and recast to a char (uint8)
		dest[pos++] = (unsigned char)strtol(buf, NULL, 16);
	}
	
}

// this function is mostly useless in a real implementation, were only using it for demonstration purposes
void hexdump(unsigned char* data, int len)
{
	int c;
	
	c=0;
	while(c < len)
	{
		printf("%.2x", data[c++]);
	}
	printf("\n");
}

// this function swaps the byte ordering of binary data, this code is slow and bloated (write your own)
void byte_swap(unsigned char* data, int len) {
	int c;
	unsigned char tmp[len];
	
	c=0;
	while(c<len)
	{
		tmp[c] = data[len-(c+1)];
		c++;
	}
	
	c=0;
	while(c<len)
	{
		data[c] = tmp[c];
		c++;
	}
}

volatile int fps;
volatile int fps_counter;
volatile int fps_measure;

void vsync_cb(void) {

    fps_counter++;
    if( fps_counter > 60 ) {
        fps = fps_measure;
        fps_measure = 0;
        fps_counter = 0;
    }

}

int main(void)
{
    init();                         // execute init()
	VSyncCallback(vsync_cb);

	// start with a block header struct
	block_header header;
	
	// you should be able to reuse these, but openssl sha256 is slow, so your probbally not going to implement this anyway
    SHA256_CTX sha256_pass1, sha256_pass2;


	// we are going to supply the block header with the values from the generation block 0
	header.version =	1;
	hex2bin(header.prev_block,		"00000000000008a3a41b85b8b29ad444def299fee21793cd8b9e567eab02cd81");
	hex2bin(header.merkle_root,		"2b12fcf1b09288fcaff797d71e950e71ae42b91e8bdb2304758dfcffc2b620e3");
	header.timestamp =	1305998791 ;
	header.bits = 		440711666;
	// header.nonce =		2504433986;
	header.nonce =		2504431986;

	// the endianess of the checksums needs to be little, this swaps them form the big endian format you normally see in block explorer
	byte_swap(header.prev_block, 32);
	byte_swap(header.merkle_root, 32);

	char hashstring[32] = "";
	char fpschar[32] = "";
	BYTE buf[SHA256_BLOCK_SIZE];
	SHA256_CTX ctx;
    while (strncmp(hashstring, "0000000", 4))                       // infinite loop
    { 
		header.nonce += 1;

		sha256_init(&ctx);
		sha256_update(&ctx, (unsigned char*)&header, sizeof(block_header));
		sha256_final(&ctx, buf);
		printf("first pass: ");
		hexdump((unsigned char*)&buf, sizeof(buf));

		BYTE buf2[SHA256_BLOCK_SIZE];
		sha256_init(&ctx);
		sha256_update(&ctx, buf, 32);
		sha256_final(&ctx, buf2);
		printf("second pass: ");
		byte_swap(buf2, 32);
		hexdump((unsigned char*)&buf2, 32);

		char hashstring2[32] = "";

		for (int i=0; i<SHA256_BLOCK_SIZE; i++){
			char lol[2] = "";
			sprintf(lol,"%02X",buf2[i]);
			strcat(hashstring2 , lol);
		}
		strcat(hashstring2 , "\0");
		strcpy(hashstring, hashstring2);


		if (header.nonce % 5 == 0){
			sprintf(fpschar,"  %d hash/sec\n\n",fps);
			FntPrint(fpschar);  // Send string to print stream
        	FntPrint(hashstring);  // Send string to print stream
			FntFlush(-1);               // Draw printe stream
        	display();                  // Execute display()
		}
		fps_measure++;
    }
	while(1){
        FntPrint(hashstring);  // Send string to print stream
		FntPrint("\n\n  tout est possible ...y"); 

        FntFlush(-1);               // Draw printe stream
        display();                  // Execute display()		
		fps_measure++;
	}
    return 0;
}
