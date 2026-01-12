# CMake generated Testfile for 
# Source directory: C:/Users/Shiva/.gemini/wisprflex/engine/native
# Build directory: C:/Users/Shiva/.gemini/wisprflex/engine/native/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(engine_test "C:/Users/Shiva/.gemini/wisprflex/engine/native/build/engine_test.exe")
set_tests_properties(engine_test PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Shiva/.gemini/wisprflex/engine/native/CMakeLists.txt;197;add_test;C:/Users/Shiva/.gemini/wisprflex/engine/native/CMakeLists.txt;0;")
add_test(whisper_smoke_test "C:/Users/Shiva/.gemini/wisprflex/engine/native/build/whisper_smoke_test.exe")
set_tests_properties(whisper_smoke_test PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Shiva/.gemini/wisprflex/engine/native/CMakeLists.txt;199;add_test;C:/Users/Shiva/.gemini/wisprflex/engine/native/CMakeLists.txt;0;")
subdirs("third_party/whisper.cpp")
