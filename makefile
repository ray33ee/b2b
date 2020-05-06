CC = gcc

FLAGS = -Wall -D _DEBUG

APP = b2b

MAIN = main

TEST = t.exe

DELETE = del /q
COPY = copy
COMPARE = ECHO n|comp


build : .\obj\$(MAIN).o
	$(CC) $(FLAGS) -o ".\bin\$(APP)" ".\obj\$(MAIN).o" -lm

.\obj\$(MAIN).o : .\src\$(MAIN).c
	$(CC) $(FLAGS) -c ".\src\$(MAIN).c" -o ".\obj\$(MAIN).o"
	
clean : 
	$(DELETE) ".\obj\$(MAIN).o" ".\bin\$(APP).exe"

test : 
	$(COPY) ".\etc\$(TEST)" ".\etc\$(TEST).bak"
	.\bin\$(APP) ".\etc\$(TEST)"
	.\bin\$(APP) ".\etc\$(TEST).bmp"
	$(COMPARE) ".\etc\$(TEST)" ".\etc\$(TEST).bak"
	$(DELETE) ".\etc\$(TEST).bak"
	

run : 
	.\bin\$(APP) "$(TEST)"
	.\bin\$(APP) "$(TEST).bmp"
