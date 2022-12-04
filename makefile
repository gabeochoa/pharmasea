

FLAGS = -std=c++2a -Wall -Wextra -Wpedantic -Wuninitialized -Wshadow \
		-Wmost -Wconversion -g -fsanitize=address
NOFLAGS = -Wno-deprecated-volatile -Wno-missing-field-initializers \
		  -Wno-c99-extensions -Wno-unused-function -Wno-sign-conversion
INCLUDES = -Ivendor/ -Lvendor/ 
LIBS = -lGameNetworkingSockets 
CPPS = src/game.cpp src/network/webrequest.cpp src/files.cpp src/app.cpp


all:
	clang++ $(FLAGS) $(NOFLAGS) $(CPPS) `pkg-config --libs --cflags raylib` $(INCLUDES) $(LIBS) -o pharmasea && ./pharmasea

profile: 
	xctrace record --output . --template "Time Profiler" --time-limit 10s --attach `ps -ef | grep "\./pharmasea" | grep -v "grep"  | cut -d' ' -f4`
count: 
	git ls-files | grep "src" | grep -v "ui_color.h" | grep -v "vendor" | grep -v "resources" | xargs wc -l | sort -rn

countall: 
	git ls-files | xargs wc -l | sort -rn

