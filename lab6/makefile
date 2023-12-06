MAIN_FILES := main.cpp buddy.cpp block.cpp process.cpp
TEST_FILES := test.cpp buddy.cpp block.cpp
EXE_FILES := main test

run: $(MAIN_FILES)
	g++ -o $@ -g $^
	./main

main: $(MAIN_FILES)
	g++ -o $@ -g $^

test: $(TEST_FILES)
	g++ -o $@ -g $^

clean: $(EXE_FILES)
	rm -f $^
