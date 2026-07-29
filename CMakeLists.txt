cmake_minimum_required(VERSION 3.20)
project(KungFuChess CXX)

# CI/Docker-image build of the pure-logic core, kept minimal for cross-platform
# (Linux GCC/Clang) compilation. Day-to-day local dev still uses Kung-Fu-Chess.sln.
add_library(kfc_core STATIC
    Kung-Fu-Chess/src/Board.cpp
    Kung-Fu-Chess/src/RuleEngine.cpp
    Kung-Fu-Chess/src/RealTimeArbiter.cpp
    Kung-Fu-Chess/src/GameEngine.cpp
    Kung-Fu-Chess/src/BoardMapper.cpp
    Kung-Fu-Chess/src/Controller.cpp
    Kung-Fu-Chess/src/CommandProcessor.cpp
    Kung-Fu-Chess/src/ProtocolIO.cpp
    Kung-Fu-Chess/src/ClientCommand.cpp
    Kung-Fu-Chess/src/GameSnapshotJson.cpp
    Kung-Fu-Chess/src/Piece.cpp
    Kung-Fu-Chess/src/Parser.cpp
    Kung-Fu-Chess/src/CommandEnvelope.cpp
    Kung-Fu-Chess/src/SubscriptionGuard.cpp
)

target_include_directories(kfc_core PUBLIC
    Kung-Fu-Chess/include
    third_party/json
)

target_compile_features(kfc_core PUBLIC cxx_std_20)

# Networking layer: WebSocket transport (GameServer/WebSocketGateway) and the
# MessageBus-driven GameShard. UserRepository/MessageBus are abstract interfaces
# at this layer, so this target still has zero vcpkg deps — the Postgres/Redis
# backed implementations arrive in a later step.
add_library(kfc_net STATIC
    Kung-Fu-Chess/src/GameServer.cpp
    Kung-Fu-Chess/src/WebSocketGateway.cpp
    Kung-Fu-Chess/src/GameShard.cpp
)

target_include_directories(kfc_net PUBLIC
    third_party/asio/include
    third_party/websocketpp
)

target_link_libraries(kfc_net PUBLIC kfc_core)

# Matches Kung-Fu-Chess.vcxproj's PreprocessorDefinitions: standalone (non-Boost)
# Asio, and force websocketpp onto its C++11 STL path (MSVC doesn't report
# __cplusplus >= 201103L unless /Zc:__cplusplus is set, which this build isn't).
target_compile_definitions(kfc_net PUBLIC
    ASIO_STANDALONE
    _WEBSOCKETPP_CPP11_STL_
)

find_package(Threads REQUIRED)
target_link_libraries(kfc_net PUBLIC Threads::Threads)

if(WIN32)
    # Windows 7+ API level for asio's socket backend, plus the Windows socket
    # libs asio needs; both match Kung-Fu-Chess.vcxproj. No-op on Linux.
    target_compile_definitions(kfc_net PUBLIC _WIN32_WINNT=0x0601)
    target_link_libraries(kfc_net PUBLIC ws2_32 mswsock)
endif()

# Postgres/Redis-backed UserRepository and MessageBus implementations. Independent
# of the WebSocket/GameShard layer (kfc_net), so this only needs kfc_core for the
# interfaces it implements, plus the vcpkg-provided client libraries.
add_library(kfc_db_redis STATIC
    Kung-Fu-Chess/src/PostgresUserRepository.cpp
    Kung-Fu-Chess/src/PqxxSmokeCheck.cpp
    Kung-Fu-Chess/src/RedisMessageBus.cpp
    Kung-Fu-Chess/src/RedisSmokeCheck.cpp
)

target_link_libraries(kfc_db_redis PUBLIC kfc_core)

find_package(libpqxx CONFIG REQUIRED)
find_package(redis++ CONFIG REQUIRED)
target_link_libraries(kfc_db_redis PUBLIC libpqxx::pqxx redis++::redis++)
