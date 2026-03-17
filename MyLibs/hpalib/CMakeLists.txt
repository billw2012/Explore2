file(GLOB hpa_src src/*.c)
add_library(hpalib STATIC ${hpa_src})
target_include_directories(hpalib PRIVATE src)
