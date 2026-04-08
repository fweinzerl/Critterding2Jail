set(tinyxml_STAT_SRCS
    tinystr.cpp
	tinyxml.cpp
	tinyxmlerror.cpp
	tinyxmlparser.cpp
    )

add_library(tinyxml SHARED ${tinyxml_STAT_SRCS})
