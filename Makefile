all: Main.cpp BuddyAllocator.cpp Ackerman.cpp
	g++ Main.cpp BuddyAllocator.cpp Ackerman.cpp -o buddyAllocator
clean:
	rm buddyAllocator