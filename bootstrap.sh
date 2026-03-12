rm -rf external
rm -rf imgui
rm -rf quickjs

git clone --depth 1 https://github.com/mekhontsev/imgui.git
git clone --depth 1 -b v0.12.0 https://github.com/quickjs-ng/quickjs.git

mkdir external
cd external

git clone --depth 1 -b v1.17.0 https://github.com/google/googletest.git
git clone --depth 1 -b 12.1.0 https://github.com/fmtlib/fmt.git

curl -O https://gitlab.com/libeigen/eigen/-/archive/3.4.1/eigen-3.4.1.tar.gz
tar -xf eigen-3.4.1.tar.gz
mv eigen-3.4.1/Eigen Eigen
rm -rf eigen-3.4.1
rm eigen-3.4.1.tar.gz

curl -O https://archives.boost.io/release/1.90.0/source/boost_1_90_0.tar.gz
tar -xf boost_1_90_0.tar.gz
mv boost_1_90_0/boost boost
rm -rf boost_1_90_0
rm boost_1_90_0.tar.gz

curl -L https://github.com/libsdl-org/SDL/releases/download/release-3.4.2/SDL3-3.4.2.tar.gz > SDL.tar.gz
tar -xf SDL.tar.gz
mv SDL3-3.4.2 SDL
rm SDL.tar.gz
