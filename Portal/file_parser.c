/*
 * File Name : file_parser.c 
 * Author : Gaurav Tungatkar
 * Creation Date : 16-01-2011
 * Description :
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "log.h"
#include "file_parser.h"

/* file_parser() - Parse a file line by line
 * Parameters:
 * filename - name of file to parse
 * reader - user defined routine to handle the parsed line. It is fed the
 *              current parsed line
 * c - void* user defined data that will be passed to reader() 
 */
int file_parser(char *filename, int (*reader)(void *, char *line), void *c)
{
        
        FILE *fp;
        char line[MAX_LINE];
        assert(filename != NULL);
        if((fp = fopen(filename, "r")) == NULL)
        {
                LOG(stdout, "Failed to open config file\n");
                return -1;
        }
        while((fgets(line, MAX_LINE, fp) != NULL))
        {
                if(reader(c, line) == -1)
                {
                        LOG(stdout, "file parser: reader() failed\n");
                        return -1;
                }
        }
        fclose(fp);
        return 0;

}
int server_map_config(void *user_data, char *line)
{
        char *ip;
        char ws[16], as[16];
	if(isspace(line[0]) || line[0] == '#') return 0;
        printf("LINE = %s", line);
        ip = strtok(line, "  \n");
        strncpy(ws, ip, 16);
        printf("firs ip = %s\n", ws);
        ip = strtok(NULL, "\n ");
        strncpy(as, ip, 16);
        printf("second ip = %s\n", as);
}
/*int main()
{
        file_parser("test", server_map_config, NULL);
        
        return 0;
}*/
