rem This file is only needed for building the templates without Bazel.

echo #ifndef STD_VECTOR_DEFINED >VECTOR.IGP 2>NUL
echo #define STD_VECTOR_DEFINED >>VECTOR.IGP 2>NUL
echo #endif // STD_VECTOR_DEFINED >VECTOR.IGS 2>NUL

templdef "vector<string> StringVector" STD\vector.ctt vector0.h vector0.inl vector0.cpp

type VECTOR.IGP vector0.h VECTOR.IGS > STD\VECTOR 2>NUL
erase VECTOR.IGP vector0.h VECTOR.IGS >NUL 2>NUL

type vector0.inl > STD\VECTOR.INL 2>NUL
erase vector0.inl >NUL 2>NUL
type vector0.cpp > STD\VECTOR.CPP 2>NUL
erase vector0.cpp >NUL 2>NUL
