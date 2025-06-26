rem This file is only needed for building the templates without Bazel.

rem vector<string>

echo #ifndef STD_VECTOR_DEFINED >VECTOR.IGP 2>NUL
echo #define STD_VECTOR_DEFINED >>VECTOR.IGP 2>NUL
echo #endif // STD_VECTOR_DEFINED >VECTOR.IGS 2>NUL

templdef "vector<string> StringVector" STD\vector.ctt vector0.h vector0.inl vector0.cpp
templdef "vector<StringPair> StringPairVector" STD\vector.ctt vector1.h vector1.inl vector1.cpp

type VECTOR.IGP vector0.h vector1.h VECTOR.IGS > STD\VECTOR 2>NUL
erase VECTOR.IGP vector0.h vector1.h VECTOR.IGS >NUL 2>NUL

type vector0.inl vector1.inl > STD\VECTOR.INL 2>NUL
erase vector0.inl vector1.inl >NUL 2>NUL
type vector0.cpp vector1.cpp > STD\VECTOR.CPP 2>NUL
erase vector0.cpp vector1.cpp >NUL 2>NUL

rem pair<string, bool>

echo #ifndef STD_UTILITY_DEFINED >UTILITY.IGP 2>NUL
echo #define STD_UTILITY_DEFINED >>UTILITY.IGP 2>NUL
echo #endif // STD_UTILITY_DEFINED >UTILITY.IGS 2>NUL

templdef "pair<string, string> StringPair" STD\utility.ctt utility0.h utility0.inl utility0.cpp
templdef "pair<string, bool> StringBoolPair" STD\utility.ctt utility1.h utility1.inl utility1.cpp

type UTILITY.IGP utility0.h utility1.h UTILITY.IGS > STD\UTILITY 2>NUL
erase UTILITY.IGP utility0.h utility1.h UTILITY.IGS >NUL 2>NUL

type utility0.inl utility1.inl > STD\UTILITY.INL 2>NUL
erase utility0.inl utility1.inl >NUL 2>NUL
type utility0.cpp utility1.cpp > STD\UTILITY.CPP 2>NUL
erase utility0.cpp utility1.cpp >NUL 2>NUL
