#ifndef __CA51M550_H__
#define __CA51M550_H__

#include <reg51.h>
#include <intrins.h>

sfr P0      = 0x80;
sfr P1      = 0x90;
sfr P2      = 0xA0;
sfr P3      = 0xB0;

sfr P00F    = 0xC1;
sfr P01F    = 0xC2;
sfr P02F    = 0xC3;
sfr P03F    = 0xC4;
sfr P04F    = 0xC5;
sfr P05F    = 0xC6;

sfr PCON    = 0x87;
sfr TCON    = 0x88;
sfr TMOD    = 0x89;
sfr TL0     = 0x8A;
sfr TL1     = 0x8B;
sfr TH0     = 0x8C;
sfr TH1     = 0x8D;
sfr CKCON   = 0x8E;

sfr IE      = 0xA8;
sfr IP      = 0xB8;

sfr SCON    = 0x98;
sfr SBUF    = 0x99;

sfr I2CCON  = 0xD0;
sfr I2CSTA  = 0xD1;
sfr I2CDAT  = 0xD2;
sfr I2CADR  = 0xD3;

sfr EP2CON  = 0xD3;
sfr EPIF    = 0xD4;

sfr PWMEN   = 0xF1;
sfr MECON   = 0xF8;

sbit EA     = IE ^ 7;
sbit EX0    = IE ^ 0;
sbit ET0    = IE ^ 1;
sbit ES     = IE ^ 4;

sbit IT0    = TCON ^ 0;
sbit IE0    = TCON ^ 0;
sbit TR0    = TCON ^ 4;
sbit TF0    = TCON ^ 5;

sbit INT4EN = 0xBE;

#endif