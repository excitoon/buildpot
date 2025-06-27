rem This file is only needed for building the templates without Bazel.

rem STD/VECTOR: vector<string>, vector<StringPair>, vector<JSON>, vector<Route>, vector<StringJSONPair>

templdef "vector<string> StringVector" STD\vector.ctt vector@0.h vector@0.inl vector@0.cpp
templdef "vector<StringPair> StringPairVector" STD\vector.ctt vector@1.h vector@1.inl vector@1.cpp
templdef "vector<JSON> JSONVector" STD\vector.ctt vector@2.h vector@2.inl vector@2.cpp
templdef "vector<Route> RouteVector" STD\vector.ctt vector@3.h vector@3.inl vector@3.cpp
templdef "vector<StringJSONPair> StringJSONPairVector" STD\vector.ctt vector@4.h vector@4.inl vector@4.cpp

echo #ifndef STD_VECTOR_DEFINED > STD\VECTOR 2>NUL
echo #define STD_VECTOR_DEFINED >> STD\VECTOR 2>NUL

type vector@0.h vector@1.h vector@2.h vector@3.h vector@4.h >> STD\VECTOR 2>NUL
erase vector@0.h vector@1.h vector@2.h vector@3.h vector@4.h >NUL 2>NUL

echo #endif // STD_VECTOR_DEFINED >> STD\VECTOR 2>NUL

type vector@0.inl vector@1.inl vector@2.inl vector@3.inl vector@4.inl > STD\VECTOR.INL 2>NUL
erase vector@0.inl vector@1.inl vector@2.inl vector@3.inl vector@4.inl >NUL 2>NUL

echo #include "string" > STD\VECTOR.CPP 2>NUL
echo #include "utility" >> STD\VECTOR.CPP 2>NUL
echo #include "vector" >> STD\VECTOR.CPP 2>NUL
echo #include "http.h" >> STD\VECTOR.CPP 2>NUL
echo #include "json.h" >> STD\VECTOR.CPP 2>NUL

type vector@0.cpp vector@1.cpp vector@2.cpp vector@3.cpp vector@4.cpp >> STD\VECTOR.CPP 2>NUL
erase vector@0.cpp vector@1.cpp vector@2.cpp vector@3.cpp vector@4.cpp >NUL 2>NUL

rem STD/UTILITY: pair<string, string>, pair<string, bool>, pair<string, JSON>

templdef "pair<string, string> StringPair" STD\utility.ctt utility@0.h utility@0.inl utility@0.cpp
templdef "pair<string, bool> StringBoolPair" STD\utility.ctt utility@1.h utility@1.inl utility@1.cpp
templdef "pair<string, JSON> StringJSONPair" STD\utility.ctt utility@2.h utility@2.inl utility@2.cpp

echo #ifndef STD_UTILITY_DEFINED > STD\UTILITY 2>NUL
echo #define STD_UTILITY_DEFINED >> STD\UTILITY 2>NUL

type utility@0.h utility@1.h utility@2.h >> STD\UTILITY 2>NUL
erase utility@0.h utility@1.h utility@2.h >NUL 2>NUL

echo #endif // STD_UTILITY_DEFINED >> STD\UTILITY 2>NUL

type utility@0.inl utility@1.inl utility@2.inl > STD\UTILITY.INL 2>NUL
erase utility@0.inl utility@1.inl utility@2.inl >NUL 2>NUL

echo #include "string" > STD\UTILITY.CPP 2>NUL
echo #include "utility" >> STD\UTILITY.CPP 2>NUL
echo #include "common.h" >> STD\UTILITY.CPP 2>NUL
echo #include "json.h" >> STD\UTILITY.CPP 2>NUL

type utility@0.cpp utility@1.cpp utility@2.cpp >> STD\UTILITY.CPP 2>NUL
erase utility@0.cpp utility@1.cpp utility@2.cpp >NUL 2>NUL

rem STD/MEMORY: shared_ptr<string>

templdef "shared_ptr<string> StringSharedPtr" STD\memory.ctt memory@0.h memory@0.inl memory@0.cpp

echo #ifndef STD_MEMORY_DEFINED > STD\MEMORY 2>NUL
echo #define STD_MEMORY_DEFINED >> STD\MEMORY 2>NUL

type memory@0.h >> STD\MEMORY 2>NUL
erase memory@0.h >NUL 2>NUL

echo #endif // STD_MEMORY_DEFINED >> STD\MEMORY 2>NUL

type memory@0.inl > STD\MEMORY.INL 2>NUL
erase memory@0.inl >NUL 2>NUL

echo #include "memory" > STD\MEMORY.CPP 2>NUL
echo #include "string" >> STD\MEMORY.CPP 2>NUL

type memory@0.cpp >> STD\MEMORY.CPP 2>NUL
erase memory@0.cpp >NUL 2>NUL
