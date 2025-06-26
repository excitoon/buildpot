rem This file is only needed for building the templates without Bazel.

rem vector<T>

templdef "vector<string> StringVector" STD\vector.ctt vector0.h vector0.inl vector0.cpp
templdef "vector<StringPair> StringPairVector" STD\vector.ctt vector1.h vector1.inl vector1.cpp
templdef "vector<JSON> JSONVector" STD\vector.ctt vector2.h vector2.inl vector2.cpp
templdef "vector<Route> RouteVector" STD\vector.ctt vector3.h vector3.inl vector3.cpp
templdef "vector<StringJSONPair> StringJSONPairVector" STD\vector.ctt vector4.h vector4.inl vector4.cpp

echo #ifndef STD_VECTOR_DEFINED > STD\VECTOR 2>NUL
echo #define STD_VECTOR_DEFINED >> STD\VECTOR 2>NUL

type vector0.h vector1.h vector2.h vector3.h vector4.h >> STD\VECTOR 2>NUL
erase vector0.h vector1.h vector2.h vector3.h vector4.h >NUL 2>NUL

echo #endif // STD_VECTOR_DEFINED >> STD\VECTOR 2>NUL

type vector0.inl vector1.inl vector2.inl vector3.inl vector4.inl > STD\VECTOR.INL 2>NUL
erase vector0.inl vector1.inl vector2.inl vector3.inl vector4.inl >NUL 2>NUL

echo #include "string" > STD\VECTOR.CPP 2>NUL
echo #include "utility" >> STD\VECTOR.CPP 2>NUL
echo #include "vector" >> STD\VECTOR.CPP 2>NUL
echo #include "http.h" >> STD\VECTOR.CPP 2>NUL
echo #include "json.h" >> STD\VECTOR.CPP 2>NUL

type vector0.cpp vector1.cpp vector2.cpp vector3.cpp vector4.cpp >> STD\VECTOR.CPP 2>NUL
erase vector0.cpp vector1.cpp vector2.cpp vector3.cpp vector4.cpp >NUL 2>NUL

rem pair<T, U>

templdef "pair<string, string> StringPair" STD\utility.ctt utility0.h utility0.inl utility0.cpp
templdef "pair<string, bool> StringBoolPair" STD\utility.ctt utility1.h utility1.inl utility1.cpp
templdef "pair<string, JSON> StringJSONPair" STD\utility.ctt utility2.h utility2.inl utility2.cpp

echo #ifndef STD_UTILITY_DEFINED > STD\UTILITY 2>NUL
echo #define STD_UTILITY_DEFINED >> STD\UTILITY 2>NUL

type utility0.h utility1.h utility2.h >> STD\UTILITY 2>NUL
erase utility0.h utility1.h utility2.h >NUL 2>NUL

echo #endif // STD_UTILITY_DEFINED >> STD\UTILITY 2>NUL

type utility0.inl utility1.inl utility2.inl > STD\UTILITY.INL 2>NUL
erase utility0.inl utility1.inl utility2.inl >NUL 2>NUL

echo #include "string" > STD\UTILITY.CPP 2>NUL
echo #include "utility" >> STD\UTILITY.CPP 2>NUL
echo #include "common.h" >> STD\UTILITY.CPP 2>NUL
echo #include "json.h" >> STD\UTILITY.CPP 2>NUL

type utility0.cpp utility1.cpp utility2.cpp >> STD\UTILITY.CPP 2>NUL
erase utility0.cpp utility1.cpp utility2.cpp >NUL 2>NUL

rem shared_ptr<T>

templdef "shared_ptr<string> StringSharedPtr" STD\memory.ctt memory0.h memory0.inl memory0.cpp

echo #ifndef STD_MEMORY_DEFINED > STD\MEMORY 2>NUL
echo #define STD_MEMORY_DEFINED >> STD\MEMORY 2>NUL

type memory0.h >> STD\MEMORY 2>NUL
erase memory0.h >NUL 2>NUL

echo #endif // STD_MEMORY_DEFINED >> STD\MEMORY 2>NUL

type memory0.inl > STD\MEMORY.INL 2>NUL
erase memory0.inl >NUL 2>NUL

echo #include "memory" > STD\MEMORY.CPP 2>NUL
echo #include "string" >> STD\MEMORY.CPP 2>NUL

type memory0.cpp >> STD\MEMORY.CPP 2>NUL
erase memory0.cpp >NUL 2>NUL
