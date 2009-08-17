all: hemi

hemi: src/hemi.o

src/hemi.o:
	g++ -Ideps/boost -Ideps/v8/include src/hemi.cpp -o bin/hemi -Ldeps/boost/stage/lib -Ldeps/v8 -lboost_system -lboost_filesystem -lboost_program_options -lv8 -lpthread
