CC = gcc

FLAGS = -Wall -D _DEBUG

APP = tobmp

MAIN = main

build : $(MAIN).o
	$(CC) $(FLAGS) -o "$(APP)" "$(MAIN).o" -lm

$(MAIN).o : $(MAIN).c
	$(CC) $(FLAGS) -c "$(MAIN).c"
	
clean : 
	del /q "$(MAIN).o" "$(APP).exe"

run : 
	$(APP) ".\files\t.exe.bmp"
