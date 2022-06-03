all:
	-cp ${PICO_SDK_PATH}/external/pico_sdk_import.cmake .
	-mkdir build
	cd build && cmake ..
	cd build && make

clean:
	rm -rf build
