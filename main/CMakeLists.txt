set(LIBRARY_SRCS
	libs/adc.cpp
	libs/thermistor.c
	libs/SD_utils.cpp
	../components/lora/lora.c
	libs/TinyGPSPlus/src/TinyGPS++.cpp
	libs/ADS1115-Lite/src/ADS1115_lite.cpp
	)
set(srcs ${LIBRARY_SRCS} main.cpp)
set(include_dirs
	libs/
	libs/TinyGPSPlus/src/
	libs/ADS1115-Lite/src/
	../components/lora/
	../components/lora/include/
	)
idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS ${include_dirs}
)
