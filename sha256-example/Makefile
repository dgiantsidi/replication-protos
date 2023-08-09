all := clean main


main: clean
	$(CXX) -Wall -fsanitize=address  $@.cpp -o $@ -lcrypto -fsanitize=address

clean:
	rm -f main
