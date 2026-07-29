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
