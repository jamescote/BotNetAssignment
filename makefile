CC = g++
CFLAGS  = -g -Wall -l crypto

controller: controller.o connection.o
	$(CC) $(CFLAGS) -o conbot controller.o connection.o

bot: bot.o connection.o
	$(CC) $(CFLAGS) -o bot bot.o connection.o

controller.o:  controller.cpp
	$(CC) $(CFLAGS) -c controller.cpp

bot.o:  bot.cpp
	$(CC) $(CFLAGS) -c bot.cpp

connection.o:  connection.cpp connection.h
	$(CC) $(CFLAGS) -c connection.cpp

clean:
	$(RM) bot conbot *.o *~
