CC = gcc

FLAGS = -Wall

APP = tobmp

MAIN = main

build : $(MAIN).o
	$(CC) $(FLAGS) -o "$(APP)" "$(MAIN).o"

$(MAIN).o : $(MAIN).c
	$(CC) $(FLAGS) -c "$(MAIN).c"
	
clean : 
	del /q "$(MAIN).o" "$(APP).exe"

run : 
	$(APP) -bmp .\files\inter.bmp .\files\test.exe
	$(APP) -bin .\files\test1.exe .\files\inter.bmp
