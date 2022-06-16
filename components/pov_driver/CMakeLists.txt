idf_component_register(SRCS "pov_driver.c"
                    INCLUDE_DIRS "include"
                    REQUIRES img_protocol apa102_driver
                    PRIV_REQUIRES pov_timer)
