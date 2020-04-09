SHELL=cmd
PORTN=$(shell type COMPORT.inc)
CC=avr-gcc
CPU=-mmcu=atmega328p
COPT=-g -Os -Wall $(CPU)
OBJS=project2.o usart.o

ADCtest.elf: $(OBJS)
	avr-gcc $(CPU) $(OBJS) -o project2.elf
	avr-objcopy -j .text -j .data -O ihex project2.elf project2.hex
	@echo done!
	
project2.o: project2.c usart.h
	avr-gcc $(COPT) -c project2.c

usart.o: usart.c usart.h
	avr-gcc $(COPT) -c usart.c

clean:
	@del *.hex *.elf $(OBJS) 2>nul

LoadFlash:
	@Taskkill /IM putty.exe /F 2>nul| wait 500
	spi_atmega -p -v -crystal project2.hex

putty:
	@Taskkill /IM putty.exe /F 2>nul| wait 500
	C:\PuTTY\putty.exe -serial $(PORTN) -sercfg 19200,8,n,1,N -v

dummy: project2.hex
	@echo Hello dummy!
	