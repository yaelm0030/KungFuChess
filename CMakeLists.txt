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

# Full doctest suite, built against the three engine libs above (as opposed to
# Kung-Fu-Chess.Tests.vcxproj, which recompiles the engine .cpp files directly).
add_executable(kfc_tests
    Kung-Fu-Chess.Tests/src/TestMain.cpp
    Kung-Fu-Chess.Tests/src/Types/CellTests.cpp
    Kung-Fu-Chess.Tests/src/Board/BoardTests.cpp
    Kung-Fu-Chess.Tests/src/BoardMapper/BoardMapperTests.cpp
    Kung-Fu-Chess.Tests/src/Parser/ParseBoardTests.cpp
    Kung-Fu-Chess.Tests/src/Parser/BoardToStringTests.cpp
    Kung-Fu-Chess.Tests/src/Parser/RoundTripTests.cpp
    Kung-Fu-Chess.Tests/src/GameEngine/GameOverTests.cpp
    Kung-Fu-Chess.Tests/src/GameEngine/JumpTests.cpp
    Kung-Fu-Chess.Tests/src/GameEngine/PromotionTests.cpp
    Kung-Fu-Chess.Tests/src/GameEngine/TickTests.cpp
    Kung-Fu-Chess.Tests/src/GameEngine/PrintTests.cpp
    Kung-Fu-Chess.Tests/src/GameEngine/CooldownTests.cpp
    Kung-Fu-Chess.Tests/src/GameEngine/MoveHistoryTests.cpp
    Kung-Fu-Chess.Tests/src/GameEngine/SnapshotTests.cpp
    Kung-Fu-Chess.Tests/src/GameSnapshot/PositionJsonTests.cpp
    Kung-Fu-Chess.Tests/src/GameSnapshot/PixelPositionJsonTests.cpp
    Kung-Fu-Chess.Tests/src/GameSnapshot/EnumJsonTests.cpp
    Kung-Fu-Chess.Tests/src/GameSnapshot/PieceSnapshotJsonTests.cpp
    Kung-Fu-Chess.Tests/src/GameSnapshot/MoveRecordJsonTests.cpp
    Kung-Fu-Chess.Tests/src/GameSnapshot/GameSnapshotJsonTests.cpp
    Kung-Fu-Chess.Tests/src/Piece/KingTests.cpp
    Kung-Fu-Chess.Tests/src/Piece/QueenTests.cpp
    Kung-Fu-Chess.Tests/src/Piece/RookTests.cpp
    Kung-Fu-Chess.Tests/src/Piece/BishopTests.cpp
    Kung-Fu-Chess.Tests/src/Piece/KnightTests.cpp
    Kung-Fu-Chess.Tests/src/Piece/PawnTests.cpp
    Kung-Fu-Chess.Tests/src/Piece/PieceFactoryTests.cpp
    Kung-Fu-Chess.Tests/src/Piece/CanPassThroughUnitsTests.cpp
    Kung-Fu-Chess.Tests/src/RuleEngine/RuleEngineTests.cpp
    Kung-Fu-Chess.Tests/src/Controller/ControllerTests.cpp
    Kung-Fu-Chess.Tests/src/CommandProcessor/CommandProcessorTests.cpp
    Kung-Fu-Chess.Tests/src/ProtocolIO/ProtocolIOTests.cpp
    Kung-Fu-Chess.Tests/src/RealTimeArbiter/RealTimeArbiterTests.cpp
    Kung-Fu-Chess.Tests/src/ClientCommand/ClientCommandTests.cpp
    Kung-Fu-Chess.Tests/src/GameServer/GameServerTests.cpp
    Kung-Fu-Chess.Tests/src/GameServer/GameServerLifecycleTests.cpp
    Kung-Fu-Chess.Tests/src/GameServer/GameServerBroadcastTests.cpp
    Kung-Fu-Chess.Tests/src/GameServer/GameServerColorAssignmentTests.cpp
    Kung-Fu-Chess.Tests/src/GameServer/GameServerInboundTests.cpp
    Kung-Fu-Chess.Tests/src/GameServer/GameServerUsernameTests.cpp
    Kung-Fu-Chess.Tests/src/UserRepository/FakeUserRepositoryTests.cpp
    Kung-Fu-Chess.Tests/src/MessageBus/InProcessMessageBusTests.cpp
    Kung-Fu-Chess.Tests/src/CommandEnvelope/CommandEnvelopeTests.cpp
    Kung-Fu-Chess.Tests/src/GameShard/GameShardTests.cpp
    Kung-Fu-Chess.Tests/src/GameShard/GameShardLifecycleTests.cpp
    Kung-Fu-Chess.Tests/src/WebSocketGateway/WebSocketGatewayColorAssignmentTests.cpp
    Kung-Fu-Chess.Tests/src/WebSocketGateway/WebSocketGatewayBroadcastTests.cpp
    Kung-Fu-Chess.Tests/src/WebSocketGateway/WebSocketGatewayInboundTests.cpp
    Kung-Fu-Chess.Tests/src/WebSocketGateway/WebSocketGatewayLifecycleTests.cpp
    Kung-Fu-Chess.Tests/src/WebSocketGateway/WebSocketGatewayUsernameTests.cpp
    Kung-Fu-Chess.Tests/src/RedisSmokeCheck/RedisUriTests.cpp
    Kung-Fu-Chess.Tests/src/SubscriptionGuard/SubscriptionGuardTests.cpp
)

target_include_directories(kfc_tests PRIVATE
    Kung-Fu-Chess.Tests/include
    Kung-Fu-Chess.Tests/src
)

target_link_libraries(kfc_tests PRIVATE kfc_core kfc_net kfc_db_redis)

target_compile_features(kfc_tests PRIVATE cxx_std_20)
