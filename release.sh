mkdir -p dist &&
cd bin
tar -cvf ../dist/deimos.tar \
    --exclude user \
    --exclude apps/axios.py \
    --exclude apps/changed.py \
    --exclude conf/axios.txt \
    --exclude conf/wifi.txt \
    $(ls) &&
cd .. &&
gh release create ""\
    dist/deimos.tar \
    $@