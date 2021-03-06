/*
 * File Name : crc32.c 
 * Author : Gaurav Tungatkar
 * Creation Date : 12-04-2011
 * Description :
 *
 */
/** Reverses bit order. MSB -> LSB and LSB -> MSB. */
#include <stdio.h>
unsigned int reverse(unsigned int x) {
    unsigned int ret = 0, i;
    for (i=0; i<32; ++i) {
        if (x&0x1 != 0) {
            ret |= (0x80000000 >> i);
        }
        else {}
        x = (x >> 1);
    }
    return ret;
}

unsigned int crc32(unsigned char* message, unsigned int msgsize, unsigned int crc) {
    unsigned int i, j; // byte counter, bit counter
    unsigned int byte;
    unsigned int poly = 0x04C11DB7;
    i = 0;
    for (i=0; i<msgsize; ++i) {
        byte = message[i];       // Get next byte.
        byte = reverse(byte);    // 32-bit reversal.
        for (j=0; j<= 7; ++j) {  // Do eight times.
            if ((int)(crc ^ byte) < 0)
                crc = (crc << 1) ^ poly;
            else crc = crc << 1;
            byte = byte << 1;    // Ready next msg bit.
        }
    }
    //return reverse(~crc);
    return crc;
}
/*
int main()
{
        unsigned int csum = 0, i = 0;
	unsigned char buf[12], buf1[6], buf2[6];
	char c = 'a';
	for(i = 0; i < 12; i++)
	{
		buf[i] = i + 0x99999;
		
	}
	printf("buf = %s\n", buf);
	for(i = 0; i < 6; i++)
	{
		buf1[i] = i + 0x99999;
	}
	printf("buf1 = %s\n", buf1);
	for(i = 6; i < 12; i++)
	{
		buf2[i-6] = i+ 0x99999;
	}

	printf("buf2 = %s\n", buf2);
        csum = crc32(buf, 12, 0);
        printf("crc32 1 = %u\n", csum);
        csum = 0;
        csum = crc32(buf1, 6, 0);
        csum = crc32(buf2, 6, csum);
        printf("crc32 2 = %u\n", csum);

        return 0;
} 
*/
