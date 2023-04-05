#切换高版本G++ source scl_source enable devtoolset-9
server:main.cpp
	#source scl_source enable devtoolset-9 
	g++ -o $@ $^ -std=c++14 -lpthread -g `mysql_config --cflags --libs`
clean:
	rm -f server
