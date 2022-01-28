hello:
	echo "hello"

debug:
	mkdir -p target/debug && cd target/debug && cmake ../.. && make

release:
	mkdir -p target/release && cd target/release && cmake -DCMAKE_BUILD_TYPE=RELEASE ../.. && make
