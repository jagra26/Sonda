set(LIBRARY_SRCS
	libs/adc.c
	libs/thermistor.c
	libs/SD_utils.cpp
	../components/lora/lora.c
	libs/TinyGPSPlus/src/TinyGPS++.cpp
	)
set(srcs ${LIBRARY_SRCS} main.cpp)
set(include_dirs
	libs/
	libs/TinyGPSPlus/src/
	../components/lora/
	../components/lora/include/
	)
idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS ${include_dirs}
)
