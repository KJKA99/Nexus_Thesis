set(includes
    "C:/Users/kebba/esp/Nexus_IoT/components/arduino/cores/esp32"
    "C:/Users/kebba/esp/Nexus_IoT/third_party/golioth-firmware-sdk/include"
)

set(srcs
    "main.cpp"
)

set (deps
     "golioth_sdk"
     #"Arduino"
)

idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    #REQUIRES ${deps}
)
