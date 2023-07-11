project(CryNetwork)

add_definitions(-D_USRDLL -DCRYNETWORK_EXPORTS)

set(SRC_FILES
    "../PunkBuster/pbcl.cpp"
    "../PunkBuster/pbmd5.cpp"
    "../PunkBuster/pbsdk.cpp"
    "../PunkBuster/pbsv.cpp"
    "../PunkBuster/pbsv.cpp"
    "CCPEndpoint.cpp"
    "Client.cpp"
    "ClientLocal.cpp"
    "ClientStateMachine.cpp"
    "CompressionHelper.cpp"
    "CryNetwork.cpp"
    "CTPEndpoint.cpp"
    "CTPEndpointGNB.cpp"
    "DatagramSocket.cpp"
    "DefenceWall.cpp"
    "NETServerSnooper.cpp"
    "Network.cpp"
    "NewUbisoftCDKey.cpp"
    "NewUbisoftClient.cpp"
    "NewUbisoftMSClient.cpp"
    "NewUbisoftRegServer.cpp"
    "PunkBusterInterface.cpp"
    "RConSystem.cpp"
    "ScriptObjectNewUbisoftClient.cpp"
    "Server.cpp"
    "ServerSlot.cpp"
    "ServerSnooper.cpp"
    "ServerStateMachine.cpp"
    "StaticCharCompressor.cpp"
    "StdAfx.cpp"
    "UbisoftMemory.cpp"
)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/SourceCode/CryCommon
)

target_link_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME})
