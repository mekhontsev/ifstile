rmdir external /s/q
rmdir imgui /s/q
rmdir quickjs /s/q

git clone --depth 1 https://github.com/mekhontsev/imgui.git
git clone --depth 1 -b v0.12.0 https://github.com/quickjs-ng/quickjs.git

mkdir external
cd external

git clone --depth 1 -b v1.17.0 https://github.com/google/googletest.git
git clone --depth 1 -b 12.1.0 https://github.com/fmtlib/fmt.git

curl -O https://gitlab.com/libeigen/eigen/-/archive/3.4.1/eigen-3.4.1.zip
tar -xf eigen-3.4.1.zip
move eigen-3.4.1/Eigen Eigen
rmdir eigen-3.4.1  /s/q
del eigen-3.4.1.zip

curl -O https://archives.boost.io/release/1.90.0/source/boost_1_90_0.zip
tar -xf boost_1_90_0.zip
move boost_1_90_0/boost boost
rmdir boost_1_90_0  /s/q
del boost_1_90_0.zip

curl -L https://github.com/libsdl-org/SDL/releases/download/release-3.4.2/SDL3-3.4.2.zip > SDL.zip
tar -xf SDL.zip
ren SDL3-3.4.2 SDL
del SDL.zip
