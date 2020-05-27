//gcc -Wall -o rtc rtc.c -lwiringPi -lwiringPiDev

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
//---------------------------------------------------

#define RTC_SECS 0
#define RTC_MINS 1
#define RTC_HOURS 2
#define RTC_DATE 3
#define RTC_MONTH 4
#define RTC_DAY 5
#define RTC_YEAR 6
#define RTC_WP 7
#define RTC_TC 8
#define RTC_BM 31

static unsigned int masks[] = {0x7F, 0x7F, 0x3F, 0x3F, 0x1F, 0x07, 0xFF};

/*
	0x7F 01111111
	0x3F 00111111
	0x1F 00011111
	0x07 00000111
	0xFF 11111111
*/
int channel = 1;

int setLinuxClock(void);
void ds1302clockRead(int clockdata[8]);
int bcdToD(unsigned int byte, unsigned int mask);
int setDSClock(void);
int dToBcd(int tmp);
int getbat(void);
int setbl(void);
int getbl(void);
int setvol(void);
int getvol(void);
int batterydead = 0;
void delayms(int ms)
{
	delay(ms);
}
int main(int argc, char *argv[])
{
	int i;
	int clock[8];
	wiringPiSPISetup(0, 500000);

	if (argc == 2)
	{
		if (strcmp(argv[1], "-slc") == 0)
		{
			return setLinuxClock(); //读取DS1302内的时间来设置Linux的时间
		}
		else if (strcmp(argv[1], "-sdsc") == 0)
		{
			return setDSClock(); //读取Linux的时间来设置DS1302的时间
		}
		else if (strcmp(argv[1], "-getvol") == 0)
		{
			return getvol(); //读取volume
		}
		else if (strcmp(argv[1], "-setvol") == 0)
		{
			return setvol(); //set volume
		}
		else if (strcmp(argv[1], "-getbl") == 0)
		{
			printf("command is get bl:");
			return getbl(); //读取backlight
		}
		else if (strcmp(argv[1], "-setbl") == 0)
		{
			return setbl(); //set backlight
		}
		else if (strcmp(argv[1], "-getbat") == 0)
		{
			return getbat(); //读取bat value
		}
		else if (strcmp(argv[1], "-gettim") == 0)
		{
			ds1302clockRead(clock); //读取bat value
			return 0;
		}
		else
		{
			setLinuxClock();
			while (1)
			{

				getvol(); //读取volume

				getbl(); //读取backlight

				getbat(); //读取bat value

				printf("loop: %d \n", i);
				sleep(1);
				//ds1302clockRead(clock);//读取DS1302内的时间
				//由于DS1302内的数据是BCD编码，所以需要转化为十进制后才能输出
				//printf(" %2d:%02d:%02d", bcdToD(clock[2], masks[2]), bcdToD(clock[1], masks[1]), bcdToD(clock[0], masks[0]));
				//printf(" %2d/%02d/%04d", bcdToD(clock[3], masks[3]), bcdToD(clock[4], masks[4]), bcdToD(clock[6], masks[6]) + 2000);
				//printf("\n");
				//sleep(1);
				i++;
				if (i > 20)
				{
					i = 0;
					setDSClock();
					sleep(1);
				}
			}
		}
	}

	setLinuxClock();
	return 0;
}

int setLinuxClock(void)
{
	char dateTime[20];
	char command[64];
	int clock[8];
	printf("Setting the Linux Clock from the RTC... ");
	fflush(stdout); //冲洗流中的信息
	ds1302clockRead(clock);
	// [MMDDhhmm[[CC]YY][.ss]]
	// 输出格式
	sprintf(dateTime, "%02d%02d%02d%02d%02d%02d.%02d",
			bcdToD(clock[RTC_MONTH], masks[RTC_MONTH]),
			bcdToD(clock[RTC_DATE], masks[RTC_DATE]),
			bcdToD(clock[RTC_HOURS], masks[RTC_HOURS]),
			bcdToD(clock[RTC_MINS], masks[RTC_MINS]),
			20,
			bcdToD(clock[RTC_YEAR], masks[RTC_YEAR]),
			bcdToD(clock[RTC_SECS], masks[RTC_SECS]));
	sprintf(command, "sudo date %s", dateTime); //字符串格式化
	//printf("/bin/date %s", dateTime);
	system(command); //向Linux输出命令
	return 0;
}

void ds1302clockRead(int clockdata[8])
{ //read clock from ds1302  低位是秒
	int i;
	unsigned char cmdread[] = {0xbe, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //0xBE read time 0xBF set time

	wiringPiSPIDataRW(0, cmdread, 1);
	delayms(100);
	delayms(100);
	delayms(100);
	delayms(100);

	for (i = 0; i < 7; i++)
	{

		wiringPiSPIDataRW(0, &cmdread[i + 1], 1);
		clockdata[i] = cmdread[i + 1];
		delayms(10); //clockdata[i] = cmdread[i+1];//读取时钟数据

		//printf("%d %d\n", i, clockdata[i]);
	}

	printf(" %2d:%02d:%02d", bcdToD(clockdata[2], masks[2]), bcdToD(clockdata[1], masks[1]), bcdToD(clockdata[0], masks[0]));
	printf(" %2d/%02d/%04d", bcdToD(clockdata[3], masks[3]), bcdToD(clockdata[4], masks[4]), bcdToD(clockdata[6], masks[6]) + 2000);
	printf("\n");
	//	printf("%d",clockdata[2]);
}

int bcdToD(unsigned int byte, unsigned int mask)
{
	byte &= mask;
	int b1 = (byte & 0xF0) >> 4; //取高四位作为十进制的十位数
	int b2 = byte & 0x0F;		 //取低四位作为十进制的个位数
	return b1 * 10 + b2;
}

int setDSClock(void)
{
	struct tm *t; //Linux时间结构体
	time_t now;
	int i;
	int clock[8];
	unsigned char cmdwrite[] = {0xbf, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //0xBE read time 0xBF set time
	printf("Setting the clock in the RTC from Linux time... ");
	time(&now);			 //取得当前时间UTC秒数，无时区转换
	t = localtime(&now); //获取当前时间结构，UTC时间，无时区转换

	clock[0] = dToBcd(t->tm_sec); // seconds
	clock[1] = dToBcd(t->tm_min); // mins
	clock[2] = t->tm_hour - 8;
	if (clock[2] < 0)
		clock[2] = clock[2] + 24;
	clock[2] = dToBcd(clock[2]);		 // hours
	clock[3] = dToBcd(t->tm_mday);		 // date
	clock[4] = dToBcd(t->tm_mon + 1);	 // months 0-11 --> 1-12
	clock[5] = dToBcd(t->tm_wday + 1);	 // weekdays (sun 0)
	clock[6] = dToBcd(t->tm_year - 100); // years
	clock[7] = 0;						 // W-Protect off

	for (i = 0; i < 8; i++)
	{

		cmdwrite[i + 1] = clock[i];
	}

	wiringPiSPIDataRW(0, cmdwrite, 1);
	delayms(100);
	wiringPiSPIDataRW(0, &cmdwrite[1], 1);
	delayms(5);
	wiringPiSPIDataRW(0, &cmdwrite[2], 1);
	delayms(5);
	wiringPiSPIDataRW(0, &cmdwrite[3], 1);
	delayms(5);
	wiringPiSPIDataRW(0, &cmdwrite[4], 1);
	delayms(5);
	wiringPiSPIDataRW(0, &cmdwrite[5], 1);
	delayms(5);
	wiringPiSPIDataRW(0, &cmdwrite[6], 1);
	delayms(5);
	wiringPiSPIDataRW(0, &cmdwrite[7], 1);
	//printf("%d", clock[2]);
	printf("OK\n");
	return 0;
}

int dToBcd(int tmp)
{
	return ((tmp / 10) << 4) + tmp % 10;
}

int getvol(void)
{
	unsigned char cmdread[] = {194, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	printf("Get Current Volume: ");
	wiringPiSPIDataRW(0, cmdread, 1);
	delayms(100);
	wiringPiSPIDataRW(0, &cmdread[1], 1);
	delayms(10);
	printf("%d \n", cmdread[1]);
	FILE *fp;
	fp = fopen("/dev/shm/volume", "w");
	fprintf(fp, "%d\n", (int)((double)cmdread[1]/255*100));
	fclose(fp);
	return 0;
}
int setvol(void)
{
	unsigned char cmdread[] = {195, 0xf0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	printf("Set Current Volume to 0xF0 \n ");
	wiringPiSPIDataRW(0, cmdread, 1);
	delayms(100);
	wiringPiSPIDataRW(0, &cmdread[1], 1);
	delayms(10);
	return 0;
}
int getbl(void)
{
	unsigned char cmdread[] = {192, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	printf("Get Current Backlight: ");
	wiringPiSPIDataRW(0, cmdread, 1);
	delayms(100);
	wiringPiSPIDataRW(0, &cmdread[1], 1);
	delayms(10);
	printf("%d \n", cmdread[1]);
	FILE *fp;
	fp = fopen("/dev/shm/backlight", "w");
	fprintf(fp, "%d\n", cmdread[1]);
	fclose(fp);
	return 0;
}
int setbl(void)
{
	unsigned char cmdread[] = {193, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	printf("Set Current Backlight to 100 \n ");
	wiringPiSPIDataRW(0, cmdread, 1);
	delayms(100);
	wiringPiSPIDataRW(0, &cmdread[1], 1);
	delayms(10);
	return 0;
}
int getbat(void)
{
	unsigned char cmdread[] = {196, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	//printf("Get Current Battery: ");
	wiringPiSPIDataRW(0, cmdread, 1);
	delayms(100);
	wiringPiSPIDataRW(0, &cmdread[1], 1);
	delayms(10);
	printf("batterylevel: %d \n", 25 * cmdread[1]);
	int socketfd;
	struct sockaddr_in addr;
	socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(55355);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	//sendto(socketfd, "SAVE_STATE", 11, 0, (struct sockaddr *)&addr, sizeof(addr));
	FILE *fp;
	fp = fopen("/dev/shm/battery", "w");
	fprintf(fp, "%d\n", 25 * cmdread[1]);
	fclose(fp);
	if (cmdread[1] == 0)
	{
		batterydead++;
		if (batterydead > 20)
		{
			sendto(socketfd, "SAVE_STATE", 11, 0, (struct sockaddr *)&addr, sizeof(addr));
			sleep(8);
			sendto(socketfd, "QUIT", 5, 0, (struct sockaddr *)&addr, sizeof(addr));
			sleep(5);
			system("sudo shutdown now");
		}
	}
	else
	{
		batterydead = 0;
	}
	close(socketfd);
	return 0;
}
