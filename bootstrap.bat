rmdir external /s/q

mkdir external
cd external

git clone --depth 1 https://github.com/mekhontsev/imgui.git
git clone --depth 1 -b v0.13.0 https://github.com/quickjs-ng/quickjs.git
git clone --depth 1 -b v1.17.0 https://github.com/google/googletest.git
git clone --depth 1 -b 12.1.0 https://github.com/fmtlib/fmt.git
git clone --depth 1 -b release-3.4.2 https://github.com/libsdl-org/SDL.git
git clone --depth 1 -b 3.4.1 https://gitlab.com/libeigen/eigen.git

curl -O https://archives.boost.io/release/1.90.0/source/boost_1_90_0.zip
tar -xf boost_1_90_0.zip
move boost_1_90_0/boost boost
rmdir boost_1_90_0  /s/q
del boost_1_90_0.zip

