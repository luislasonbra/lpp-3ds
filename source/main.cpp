/*----------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#------  This File is Part Of : ----------------------------------------------------------------------------------------#
#------- _  -------------------  ______   _   --------------------------------------------------------------------------#
#------ | | ------------------- (_____ \ | |  --------------------------------------------------------------------------#
#------ | | ---  _   _   ____    _____) )| |  ____  _   _   ____   ____   ----------------------------------------------#
#------ | | --- | | | | / _  |  |  ____/ | | / _  || | | | / _  ) / ___)  ----------------------------------------------#
#------ | |_____| |_| |( ( | |  | |      | |( ( | || |_| |( (/ / | |  --------------------------------------------------#
#------ |_______)\____| \_||_|  |_|      |_| \_||_| \__  | \____)|_|  --------------------------------------------------#
#------------------------------------------------- (____/  -------------------------------------------------------------#
#------------------------   ______   _   -------------------------------------------------------------------------------#
#------------------------  (_____ \ | |  -------------------------------------------------------------------------------#
#------------------------   _____) )| | _   _   ___   ------------------------------------------------------------------#
#------------------------  |  ____/ | || | | | /___)  ------------------------------------------------------------------#
#------------------------  | |      | || |_| ||___ |  ------------------------------------------------------------------#
#------------------------  |_|      |_| \____|(___/   ------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Licensed under the GPL License --------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Copyright (c) Nanni <lpp.nanni@gmail.com> ---------------------------------------------------------------------------#
#- Copyright (c) Rinnegatamante <rinnegatamante@gmail.com> -------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Credits : -----------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Smealum for ctrulib and ftpony src ----------------------------------------------------------------------------------#
#- StapleButter for debug font -----------------------------------------------------------------------------------------#
#- Lode Vandevenne for lodepng -----------------------------------------------------------------------------------------#
#- Jean-loup Gailly and Mark Adler for zlib ----------------------------------------------------------------------------#
#- Special thanks to Aurelio for testing, bug-fixing and various help with codes and implementations -------------------#
#-----------------------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include "include/luaplayer.h"
#include "include/luaGraphics.h"
#include "include/ftp/ftp.h"

const char *errMsg;
unsigned char *buffer;
char cur_dir[256];
char start_dir[256];

int main(int argc, char **argv)
{
	srvInit();	
	aptInit();
	gfxInitDefault();
	acInit();
	initCfgu();
	httpcInit();
	ptmInit();
	hidInit(NULL);
	irrstInit(NULL);
	aptOpenSession();
	Result ret=APT_SetAppCpuTimeLimit(NULL, 30);
	aptCloseSession();
	fsInit();
	hbInit();
	Handle fileHandle;
	u64 size;
	u32 bytesRead;
	int restore;
	
	// Set main script
	char path[256];
	if (argc > 0){
		int latest_slash = 0;
		int i=5;
		while (argv[0][i]  != '\0'){
		if (argv[0][i] == '/'){
		latest_slash = i;
		}
		i++;
		}
		strcpy(path,&argv[0][5]);
		path[latest_slash-5] = 0;
		strcpy(start_dir,path);
		strcpy(cur_dir,path); // Set current dir
		strcat(path,"/index.lua");
	}else{
		strcpy(start_dir,"/");
		strcpy(cur_dir,"/"); // Set current dir for GW Mode
		strcpy(path,"/index.lua");
	}
	
	while(aptMainLoop())
	{
		restore=0;		
		char error[2048];
		
		// Load main script
		FS_path filePath=FS_makePath(PATH_CHAR, path);
		FS_archive script=(FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
		FSUSER_OpenFileDirectly(NULL, &fileHandle, script, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
		FSFILE_GetSize(fileHandle, &size);
		buffer = (unsigned char*)(malloc((size+1) * sizeof (char)));
		FSFILE_Read(fileHandle, &bytesRead, 0x0, buffer, size);
		buffer[size]=0;
		FSFILE_Close(fileHandle);
		svcCloseHandle(fileHandle);
		errMsg = runScript((const char*)buffer, true);
		free(buffer);
		
		if (errMsg != NULL);{
		
			// Fake error to force interpreter shutdown
			if (strstr(errMsg, "lpp_exit_04")) break;
			
		}
		bool ftp_state = false;
		int connfd;
		while (restore==0){
			gspWaitForVBlank();
			RefreshScreen();
			ClearScreen(0);
			ClearScreen(1);
			strcpy(error,"Error: ");
			strcat(error,errMsg);
			if (ftp_state){ 
				u32 ip=(u32)gethostid();
				char ip_address[64];
				strcat(error,"\n\nPress A to restart\nPress B to exit\nPress Y to enable FTP server\n\nFTP state: ON\nIP: ");
				sprintf(ip_address,"%lu.%lu.%lu.%lu", ip & 0xFF, (ip>>8)&0xFF, (ip>>16)&0xFF, (ip>>24)&0xFF);
				strcat(error,ip_address);
				strcat(error,"\nPort: 5000");
				if(connfd<0)connfd=ftp_getConnection();
				else{
					int ret=ftp_frame(connfd);
					if(ret==1) connfd=-1;
				}
			}else strcat(error,"\n\nPress A to restart\nPress B to exit\nPress Y to enable FTP server\n\nFTP state: OFF");
			DebugOutput(error);
			hidScanInput();
			if(hidKeysDown() & KEY_A){
				strcpy(cur_dir,start_dir);
				restore=1;
			}else if(hidKeysDown() & KEY_B){
				restore=2;
			}else if(hidKeysDown() & KEY_Y){
				if (!ftp_state){
					u32 wifiStatus;
					if ((u32)ACU_GetWifiStatus(NULL, &wifiStatus) !=  0xE0A09D2E){
						if (wifiStatus != 0){
							ftp_init();
							connfd = -1;
							ftp_state = true;
						}
					}
				}
			}
			
			gfxFlushBuffers();
			gfxSwapBuffers();
		}
		if (ftp_state) ftp_exit();
		if (isCSND){
			CSND_shutdown();
			isCSND = false;
		}
		if (restore==2){
			break;
		}
	}
	fsExit();
	irrstExit();
	hidExit();
	ptmExit();
	hbExit();
	acExit();
	httpcExit();
	exitCfgu();
	gfxExit();
	aptExit();
	srvExit();

	return 0;
}
