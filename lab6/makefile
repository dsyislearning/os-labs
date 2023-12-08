BUDDY_FILES := main.cpp buddy.cpp block.cpp process.cpp
SLAB_FILES := main_slab.cpp slab.cpp slabcache.cpp slabobject.cpp slabpage.cpp block.cpp buddy.cpp
TEST_FILES := test.cpp buddy.cpp block.cpp
EXE_FILES := main test

run: $(BUDDY_FILES)
	g++ -o $@ -g $^
	./main

buddy: $(BUDDY_FILES)
	g++ -o $@ -g $^

slab: $(SLAB_FILES)
	g++ -o $@ -g $^

test: $(TEST_FILES)
	g++ -o $@ -g $^

clean: $(EXE_FILES)
	rm -f $^
