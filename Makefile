all: hemi

hemi: src/hemi.o

src/hemi.o:
	g++ -I/opt/local/include/ -Ideps/v8/include src/hemi.cpp -o bin/hemi -L/opt/local/lib/ -Ldeps/v8 -lboost_system-mt -lboost_filesystem-mt -lv8 -lpthread
