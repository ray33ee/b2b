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
	$(APP) .\file.zip
