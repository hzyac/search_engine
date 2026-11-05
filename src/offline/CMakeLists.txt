add_library(search_offline
    dataPreparation.cpp
    dictProducer.cpp
    webPage.cpp
    invertedIndex.cpp
)

target_link_libraries(search_offline
    PUBLIC
        search_text
        SearchThirdParty::simhash
)
