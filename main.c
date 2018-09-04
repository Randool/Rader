#include "support.h"
#include "store.h"
#include "servo.h"
#include "serial.h"

#define FOCS 11059200L
#define BAUD 9600
#define TM   (65536 - FOCS/BAUD/4)

sbit sLED = P3^3;
sbit echo = P1^0;
sbit trig = P1^1;

uint16 distance;
uint8 recv_buff;


void setup() {
	// 舵机
	P4M1 = 0x00, P4M0 = 0xff;
	P4 = 0;

	// LED光效
	P0M1 = 0x00, P0M0 = 0xff;	// 8段可亮
	P2M1 = 0x00, P2M0 = 0x08;	// 8个数码管全可亮
	sLED = 1;
	P0 = 0;
	
	// 超声波——定时器0
	TMOD = 0x01;
	TH0 = 0;
	TL0 = 0;
	
	// 串口通信——定时器1
	SCON |= 0x50;
	TH1 = TM >> 8;
	TL1 = TM;
	AUXR = 0x40;
	TI = RI = 0;
	TR1 = 1;
	ES = 1;
	EA = 1;
	
	store_init();	// 存储设备的初始化
}

void getDistance() {
	TR1 = 0;
	trig = 1;
	Delay25us();
	trig = 0;
	TH0 = TL0 = 0;
	while (!echo);
	TR0 = 1;
	while (echo);
	TR0 = 0;
	distance = (TH0 * 256 + TL0) * 0.172;
	if (distance > 4000) distance = 4000;
	TR1 = 1;
}

void loop() {
	while (1) {
		getDistance();
		Duang();
		send_data(0x11, distance);
		
		// 记录距离，由于distance为16位，需要传输两次
		write_addr(pointer++, distance >> 8);
		write_addr(pointer++, distance & 0xff);
		if (pointer >= 2*direction)
			pointer = 0x00;

		step();
	}
}

void main() {
	setup();
	loop();
}

void Uart1() interrupt 4 using 1 {
	if (RI) {
		recv_buff = SBUF;
		RI = 0;
	}
	if (TI) {
		busy = 0;
		TI = 0;
	}
}