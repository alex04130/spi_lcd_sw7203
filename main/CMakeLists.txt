idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver sw7203 bq4050)
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
